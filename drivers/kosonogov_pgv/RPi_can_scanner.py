import os
import time
import struct
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler
import numpy as np
import cv2
import zxingcpp
import can
from picamera2 import Picamera2
import board
import neopixel
#sudo systemctl stop can-scanner.service выключение
#sudo systemctl start can-scanner.service включение

# =========================================================================
# НАСТРОЙКИ CANopen И iRAYPLE
# =========================================================================
CAN_INTERFACE = "can0"
NODE_ID = 0x01                  # Адрес узла CANopen (1..127)

COB_ID_NMT = 0x000              # Прием стандартных команд NMT
COB_ID_RPDO1 = 0x200 + NODE_ID  # Прием прямых команд iRAYPLE (0x201)
COB_ID_TPDO2 = 0x280 + NODE_ID  # Передача Угла (0x281, байты 4-5)
COB_ID_TPDO3 = 0x380 + NODE_ID  # Передача ID и видимости (0x381, байты 0, 4-7)
COB_ID_BOOTUP = 0x700 + NODE_ID # Сообщение Boot-up (0x701)
COB_ID_SDO_RX = 0x600 + NODE_ID # Запросы от ПЛК (0x601)
COB_ID_SDO_TX = 0x580 + NODE_ID # Ответы сканера (0x581)

DEFAULT_OPERATIONAL = False     # False: старт в PRE-OPERATIONAL
ANGLE_DEADBAND_DEG = 0.2        
INHIBIT_TIME_SEC = 0.010        
STOP_SEND_NO_READ = False       

STATE_PRE_OPERATIONAL = 0x7F
STATE_OPERATIONAL = 0x05
STATE_STOPPED = 0x04

current_nmt_state = STATE_OPERATIONAL if DEFAULT_OPERATIONAL else STATE_PRE_OPERATIONAL
force_single_send = False
state_lock = threading.Lock()
running = True

# =========================================================================
# ПОДСВЕТКА (Красный цвет для OV9281 с защитой от зависания DMA)
# =========================================================================
NUM_LEDS = 12
COLOR_RED = (255, 0, 0)
COLOR_OFF = (0, 0, 0)

try:
    led_pixels = neopixel.NeoPixel(
        board.D12,  # Физический Pin 32 (GPIO 12)
        NUM_LEDS,
        brightness=1.0,
        auto_write=False,
        pixel_order=neopixel.GRB,
    )
    print("Кольцо WS2812B (Pin 32 / GPIO 12) инициализировано.")
except Exception as e:
    print(f"Ошибка инициализации кольца: {e}")
    led_pixels = None

led_lock = threading.Lock()
current_light_state = None

def set_light(state: bool):
    """Потокобезопасное переключение подсветки."""
    global led_pixels, current_light_state, led_lock
    if led_pixels is None:
        return
    with led_lock:
        if current_light_state == state:
            return
        try:
            if state:
                led_pixels.fill(COLOR_RED)
                led_pixels.show()
                current_light_state = True
                print("💡 Подсветка: ВКЛ (КРАСНЫЙ)")
            else:
                led_pixels.fill(COLOR_OFF)
                led_pixels.show()
                current_light_state = False
                print("🌑 Подсветка: ВЫКЛ")
        except Exception as e:
            print(f"Ошибка переключения светодиодов: {e}")

# =========================================================================
# КАЛИБРОВКА КАМЕРЫ OV9281
# =========================================================================
CALIB_FILE = "/home/alexander/mark/camera_calib_ov9281.npz"

if os.path.exists(CALIB_FILE):
    try:
        calib_data = np.load(CALIB_FILE)
        K_MATRIX = calib_data["K"].astype(np.float32)
        DIST_COEFFS = calib_data["D"].astype(np.float32)
    except Exception:
        K_MATRIX = np.array([[816.9941, 0.0, 575.7757], [0.0, 817.8677, 425.9962], [0.0, 0.0, 1.0]], dtype=np.float32)
        DIST_COEFFS = np.array([0.1581125, -0.328207, -0.0000222, -0.0018829, 0.115539], dtype=np.float32)
else:
    K_MATRIX = np.array([[816.9941, 0.0, 575.7757], [0.0, 817.8677, 425.9962], [0.0, 0.0, 1.0]], dtype=np.float32)
    DIST_COEFFS = np.array([0.1581125, -0.328207, -0.0000222, -0.0018829, 0.115539], dtype=np.float32)

SCALE_FACTOR = 0.5
INNER_MARGIN = -30
MARKER_ORIENTATION_OFFSET = {"10": 0.0, "00": -90.0, "01": 180.0, "11": 90.0}

# =========================================================================
# БУФЕР ВЕБ-СТРИМА (MJPEG)
# =========================================================================
output_jpeg = None
stream_lock = threading.Lock()

class StreamHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path in ['/', '/stream.mjpg']:
            self.send_response(200)
            self.send_header('Age', '0')
            self.send_header('Cache-Control', 'no-cache, private')
            self.send_header('Pragma', 'no-cache')
            self.send_header('Content-Type', 'multipart/x-mixed-replace; boundary=FRAME')
            self.end_headers()
            try:
                while running:
                    with stream_lock:
                        if output_jpeg is None:
                            time.sleep(0.01)
                            continue
                        frame = output_jpeg
                    self.wfile.write(b'--FRAME\r\n')
                    self.send_header('Content-Type', 'image/jpeg')
                    self.send_header('Content-Length', str(len(frame)))
                    self.end_headers()
                    self.wfile.write(frame)
                    self.wfile.write(b'\r\n')
                    time.sleep(0.033)
            except Exception:
                pass
        else:
            self.send_error(404)
            self.end_headers()
    def log_message(self, format, *args): return

def start_web_server():
    try:
        server = HTTPServer(('0.0.0.0', 8080), StreamHandler)
        server.serve_forever()
    except Exception as e:
        print(f"Ошибка HTTP сервера: {e}")

# =========================================================================
# ВИЖЕН
# =========================================================================
def initialize_camera():
    picam2 = Picamera2()
    config = picam2.create_video_configuration(main={"size": (1280, 800), "format": "YUV420"}, buffer_count=4)
    picam2.configure(config)
    picam2.start()
    picam2.set_controls({
        "FrameDurationLimits": (10000, 10000),
        "ExposureTime": 3000,
        "AnalogueGain": 1.0,
        "AeFlickerMode": 0
    })
    return picam2

def undistort_marker_points(pts_array):
    pts_in = np.asarray(pts_array, dtype=np.float32).reshape(-1, 1, 2)
    pts_out = cv2.undistortPoints(pts_in, cameraMatrix=K_MATRIX, distCoeffs=DIST_COEFFS, P=K_MATRIX)
    return pts_out.reshape(-1, 2)

def normalize_angle_180(angle_deg):
    return ((angle_deg + 180.0) % 360.0) - 180.0

def calc_angle_diff(a1, a2):
    delta = abs(a1 - a2) % 360.0
    return (360.0 - delta) if delta > 180.0 else delta

def compute_pair_angle(id1, id2, detected_dict):
    pt1, pt2 = detected_dict[id1]['bl_full'], detected_dict[id2]['bl_full']
    dx_px, dy_px = pt2[0] - pt1[0], -(pt2[1] - pt1[1])
    theta_meas = np.degrees(np.arctan2(dy_px, dx_px))
    col1, row1 = int(id1[1]), int(id1[0])
    col2, row2 = int(id2[1]), int(id2[0])
    theta_base = np.degrees(np.arctan2(-(row2 - row1), col2 - col1))
    return normalize_angle_180(theta_meas - theta_base)

def compute_single_angle(m_id, m_data):
    pt1, pt2 = m_data['bl_full'], m_data['tr_full']
    raw_yaw = np.degrees(np.arctan2(-(pt2[1] - pt1[1]), pt2[0] - pt1[0]))
    offset = MARKER_ORIENTATION_OFFSET.get(m_id, 0.0)
    return normalize_angle_180(raw_yaw - 45.0 - offset)

def calculate_stand_angle(detected_dict):
    for id1, id2 in [("00", "11"), ("10", "01")]:
        if id1 in detected_dict and id2 in detected_dict:
            return compute_pair_angle(id1, id2, detected_dict), 1, (id1, id2)
    for id1, id2 in [("00", "01"), ("10", "11"), ("00", "10"), ("01", "11")]:
        if id1 in detected_dict and id2 in detected_dict:
            return compute_pair_angle(id1, id2, detected_dict), 2, (id1, id2)
    for m_id, m_data in detected_dict.items():
        return compute_single_angle(m_id, m_data), 3, (m_id, None)
    return None, 0, (None, None)

def get_all_rois(frame_gray):
    h_img, w_img = frame_gray.shape
    small_gray = cv2.resize(frame_gray, (0, 0), fx=SCALE_FACTOR, fy=SCALE_FACTOR, interpolation=cv2.INTER_NEAREST)
    _, thresh = cv2.threshold(small_gray, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
    contours, _ = cv2.findContours(thresh, cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE)

    rois = []
    area_min, area_max = 35000 * (SCALE_FACTOR ** 2), 55000 * (SCALE_FACTOR ** 2)
    scale_inv = 1.0 / SCALE_FACTOR

    for cnt in contours:
        area = cv2.contourArea(cnt)
        if area_min < area < area_max:
            sx, sy, sw, sh = cv2.boundingRect(cnt)
            max_side = max(sw, sh)
            if (min(sw, sh) / max_side if max_side > 0 else 0.0) > 0.70:
                bx, by = int(sx * scale_inv), int(sy * scale_inv)
                bw, bh = int(sw * scale_inv), int(sh * scale_inv)
                x1 = min(w_img - 1, max(0, bx + INNER_MARGIN))
                y1 = min(h_img - 1, max(0, by + INNER_MARGIN))
                x2 = max(x1 + 1, min(w_img, bx + bw - INNER_MARGIN))
                y2 = max(y1 + 1, min(h_img, by + bh - INNER_MARGIN))
                roi_cropped = frame_gray[y1:y2, x1:x2]
                if roi_cropped.size > 0:
                    rois.append((roi_cropped, x1, y1, (bx, by, bw, bh)))
    return rois

def parse_tag_number(payload: str) -> int:
    digits = "".join(ch for ch in payload if ch.isdigit())
    if len(digits) > 2:
        return int(digits[2:]) & 0xFFFFFFFF
    elif len(digits) > 0:
        return int(digits) & 0xFFFFFFFF
    return 0

def scan_frame(frame_gray):
    rois = get_all_rois(frame_gray)
    detected_dict = {}
    tag_numbers = []

    for roi, x_off, y_off, box_rect in rois:
        results = zxingcpp.read_barcodes(
            roi,
            formats=zxingcpp.BarcodeFormat.DataMatrix,
            binarizer=zxingcpp.Binarizer.GlobalHistogram,
            try_invert=False
        )
        if results:
            barcode = results[0]
            payload = barcode.text.strip()
            if len(payload) >= 2 and payload[:2].isdigit():
                m_id = payload[:2]
                tag_numbers.append(parse_tag_number(payload))
                pos = barcode.position
                raw_corners = np.array([
                    [pos.top_left.x + x_off, pos.top_left.y + y_off],
                    [pos.top_right.x + x_off, pos.top_right.y + y_off],
                    [pos.bottom_right.x + x_off, pos.bottom_right.y + y_off],
                    [pos.bottom_left.x + x_off, pos.bottom_left.y + y_off]
                ], dtype=np.float32)
                undist_corners = undistort_marker_points(raw_corners)
                detected_dict[m_id] = {
                    'bl_full': (int(undist_corners[3][0]), int(undist_corners[3][1])),
                    'tr_full': (int(undist_corners[1][0]), int(undist_corners[1][1])),
                    'corners_full': undist_corners,
                    'payload': payload
                }

    if not detected_dict:
        return {"read": False, "tag_id": 0, "yaw": 0.0, "priority": 0, "count": 0,
                "detected_dict": {}, "rois": rois, "used_pair": (None, None)}

    calc_yaw, priority, used_pair = calculate_stand_angle(detected_dict)
    if calc_yaw is None:
        return {"read": False, "tag_id": 0, "yaw": 0.0, "priority": 0, "count": 0,
                "detected_dict": detected_dict, "rois": rois, "used_pair": (None, None)}

    primary_tag_id = tag_numbers[0] if len(set(tag_numbers)) == 1 else min(tag_numbers)

    return {
        "read": True,
        "tag_id": primary_tag_id,
        "yaw": calc_yaw,
        "priority": priority,
        "count": len(detected_dict),
        "detected_dict": detected_dict,
        "rois": rois,
        "used_pair": used_pair
    }

# =========================================================================
# CAN WORKER
# =========================================================================
def send_bootup(bus: can.BusABC):
    try:
        bus.send(can.Message(arbitration_id=COB_ID_BOOTUP, data=[0x00], is_extended_id=False))
    except can.CanError:
        pass

def pack_tpdo2(yaw_deg, is_read):
    """Байты 4..5: Угол * 10 (int16 BIG-ENDIAN)"""
    data = bytearray(8)
    if is_read and yaw_deg is not None:
        try:
            angle_val = int(round(float(yaw_deg) * 10.0))
            angle_val = max(-1800, min(1800, angle_val))
            struct.pack_into(">h", data, 4, angle_val) 
        except Exception:
            pass
    return bytes(data)

def pack_tpdo3(tag_id, is_read):
    """Байт 0: Visibility (маска 0x08), Байты 4..7: Tag ID (uint32 BIG-ENDIAN)"""
    data = bytearray(8)
    if is_read:
        data[0] = 0x08 
        tag_num = 0
        if tag_id is not None:
            try:
                digits = "".join(filter(str.isdigit, str(tag_id)))
                tag_num = int(digits) if digits else 0
            except Exception:
                pass
        struct.pack_into(">I", data, 4, tag_num & 0xFFFFFFFF) 
    return bytes(data)

def can_rx_worker(bus: can.BusABC):
    global current_nmt_state, force_single_send, running
    print("CAN RX поток запущен и слушает команды...")
    while running:
        try:
            msg = bus.recv(timeout=0.5)
            if msg is None: continue

            # 1. CANopen NMT Master Commands
            if msg.arbitration_id == 0x000 and len(msg.data) >= 2:
                cmd, target_node = msg.data[0], msg.data[1]
                if target_node in (NODE_ID, 0x00):
                    if cmd == 0x01:
                        with state_lock: current_nmt_state = STATE_OPERATIONAL
                        set_light(True)
                    elif cmd == 0x80:
                        with state_lock: current_nmt_state = STATE_PRE_OPERATIONAL
                        set_light(False)
                    elif cmd == 0x02:
                        with state_lock: current_nmt_state = STATE_STOPPED
                        set_light(False)
                    elif cmd == 0x81:
                        send_bootup(bus)
                        with state_lock: current_nmt_state = STATE_PRE_OPERATIONAL

            # 2. iRAYPLE & Управление подсветкой
            elif msg.arbitration_id == COB_ID_RPDO1 and len(msg.data) >= 2:
                cmd_code = (msg.data[0] << 8) | msg.data[1]
                if cmd_code == 0xFC03:
                    with state_lock: current_nmt_state = STATE_OPERATIONAL
                    set_light(True)
                elif cmd_code == 0xF807:
                    with state_lock: current_nmt_state = STATE_PRE_OPERATIONAL
                    set_light(False)
                elif cmd_code == 0xC837:
                    with state_lock: force_single_send = True
                elif cmd_code == 0x0000:
                    set_light(False)
                elif cmd_code == 0x0001:
                    set_light(True)

            # 3. SDO Запросы (ПЛК проверяет DeviceType 0x1000)
            elif msg.arbitration_id == COB_ID_SDO_RX and len(msg.data) >= 4:
                cmd, idx_low, idx_high, sub_idx = msg.data[0], msg.data[1], msg.data[2], msg.data[3]
                if cmd == 0x40 and idx_low == 0x00 and idx_high == 0x10 and sub_idx == 0x00:
                    sdo_resp = [0x43, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00]
                    try:
                        bus.send(can.Message(arbitration_id=COB_ID_SDO_TX, data=sdo_resp, is_extended_id=False))
                    except Exception:
                        pass
        except Exception:
            time.sleep(0.1)

# =========================================================================
# ГЛАВНЫЙ ЦИКЛ
# =========================================================================
def main():
    global running, force_single_send, output_jpeg

    if DEFAULT_OPERATIONAL:
        set_light(True)
    else:
        set_light(False)
        
    picam2 = initialize_camera()

    threading.Thread(target=start_web_server, daemon=True).start()
    print("Веб-сервер запущен: порт 8080")

    # Шина CAN на 1 Мбит/с
    os.system(f"sudo ip link set {CAN_INTERFACE} txqueuelen 1000 2>/dev/null")
    os.system(f"sudo ip link set {CAN_INTERFACE} up type can bitrate 1000000 restart-ms 100 2>/dev/null")
    time.sleep(0.1)

    try:
        bus = can.interface.Bus(channel=CAN_INTERFACE, interface="socketcan")
        threading.Thread(target=can_rx_worker, args=(bus,), daemon=True).start()
        send_bootup(bus)
        print("CAN шина успешно подключена (1 Mbps)")
    except Exception as e:
        print(f"Предупреждение: CAN не открылся ({e}), стрим работает")
        bus = None

    cam_cx = int(round(float(K_MATRIX[0, 2])))
    cam_cy = int(round(float(K_MATRIX[1, 2])))

    last_sent_read = None
    last_sent_tag_id = None
    last_sent_yaw = None
    last_send_time = 0.0
    total_tx_packets = 0

    frame_counter = 0
    prev_time = time.time()
    fps_val = 0.0

    try:
        while True:
            with state_lock:
                state = current_nmt_state
                send_now = force_single_send

            yuv = picam2.capture_array("main")
            frame_gray = yuv[:800, :1280]

            res = scan_frame(frame_gray)
            now = time.perf_counter()

            is_read = res["read"]
            tag_id = res["tag_id"]
            yaw = res["yaw"]

            should_transmit = False
            if state == STATE_OPERATIONAL or send_now:
                if send_now:
                    should_transmit = True
                    with state_lock:
                        force_single_send = False
                elif (now - last_send_time) >= INHIBIT_TIME_SEC:
                    if is_read:
                        if (last_sent_read is not True or tag_id != last_sent_tag_id or 
                            last_sent_yaw is None or calc_angle_diff(yaw, last_sent_yaw) >= ANGLE_DEADBAND_DEG):
                            should_transmit = True
                    else:
                        if not STOP_SEND_NO_READ and last_sent_read is not False:
                            should_transmit = True

            if should_transmit and bus is not None:
              try:
                # TPDO2 - Угол
                msg2 = can.Message(
                    arbitration_id=COB_ID_TPDO2,
                    data=pack_tpdo2(yaw, is_read),
                    is_extended_id=False,
                )
                bus.send(msg2)

                # TPDO3 - Visibility и ID
                msg3 = can.Message(
                    arbitration_id=COB_ID_TPDO3,
                    data=pack_tpdo3(tag_id, is_read),
                    is_extended_id=False,
                )
                bus.send(msg3)

                last_sent_read = is_read
                last_sent_tag_id = tag_id if is_read else None
                last_sent_yaw = yaw if is_read else None
                last_send_time = now
                total_tx_packets += 2
              except Exception as e:
                # Перехватываем и can.CanError, и OSError (105 ENOBUFS)
                # Даем шине 50 мс на опустошение буфера, чтобы не спамить в забитый сокет
                time.sleep(0.05)

            t_now_wall = time.time()
            dt = t_now_wall - prev_time
            if dt > 0: fps_val = 0.9 * fps_val + 0.1 * (1.0 / dt)
            prev_time = t_now_wall

            frame_counter += 1
            if frame_counter % 3 == 0:
                display_frame = cv2.cvtColor(frame_gray, cv2.COLOR_GRAY2BGR)

                cv2.line(display_frame, (0, cam_cy), (1279, cam_cy), (0, 255, 0), 1)
                cv2.line(display_frame, (cam_cx, 0), (cam_cx, 799), (0, 255, 0), 1)

                detected = res["detected_dict"]
                for _, _, _, (bx, by, bw, bh) in res["rois"]:
                    cv2.rectangle(display_frame, (bx, by), (bx + bw, by + bh), (0, 220, 255), 1)

                for m_id, data in detected.items():
                    pts = data['corners_full'].astype(np.int32)
                    cv2.polylines(display_frame, [pts], isClosed=True, color=(0, 255, 0), thickness=2)
                    cv2.line(display_frame, data['bl_full'], data['tr_full'], (0, 0, 255), 2)
                    cv2.circle(display_frame, data['bl_full'], 4, (0, 255, 255), -1)
                    mx, my = int(np.mean(pts[:, 0])), int(np.mean(pts[:, 1]))
                    cv2.putText(display_frame, f"ID:{m_id}", (mx - 15, my - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

                used_pair = res.get("used_pair")
                if used_pair and used_pair[0] and used_pair[1] and used_pair[0] in detected and used_pair[1] in detected:
                    cv2.line(display_frame, detected[used_pair[0]]['bl_full'], detected[used_pair[1]]['bl_full'], (255, 0, 255), 2)

                cv2.rectangle(display_frame, (10, 10), (380, 240), (20, 20, 20), -1)
                cv2.rectangle(display_frame, (10, 10), (380, 240), (0, 255, 0), 1)

                st_text = "OPERATIONAL" if state == STATE_OPERATIONAL else "PRE-OPERATIONAL"
                st_color = (0, 255, 0) if state == STATE_OPERATIONAL else (0, 165, 255)
                cv2.putText(display_frame, f"CAN State: {st_text}", (20, 35), cv2.FONT_HERSHEY_SIMPLEX, 0.55, st_color, 2)
                cv2.putText(display_frame, f"Vision FPS: {fps_val:.1f} | TX: {total_tx_packets}", (20, 65), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1)

                if is_read:
                    cv2.putText(display_frame, f"Tag ID: {tag_id} (Codes: {res['count']})", (20, 95), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
                    cv2.putText(display_frame, f"Angle: {yaw:+.2f} deg", (20, 125), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
                else:
                    cv2.putText(display_frame, "Tag ID: NO CODE IN VIEW", (20, 95), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)
                    cv2.putText(display_frame, "Angle: ---", (20, 125), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (180, 180, 180), 1)

                cv2.putText(display_frame, f"TPDO2/3 COB-ID: 0x{COB_ID_TPDO2:03X} / 0x{COB_ID_TPDO3:03X}", (20, 155), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
                cv2.putText(display_frame, f"Baudrate: 1 Mbps", (20, 185), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
                
                with led_lock:
                    lt_st = "ON (RED)" if current_light_state else "OFF"
                cv2.putText(display_frame, f"Light (Pin 32): {lt_st}", (20, 215), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 0), 1)

                _, jpeg = cv2.imencode('.jpg', display_frame, [cv2.IMWRITE_JPEG_QUALITY, 60])
                with stream_lock:
                    output_jpeg = jpeg.tobytes()

    except KeyboardInterrupt:
      print("\nОстановка по Ctrl+C...")
    except Exception as e:
      import traceback

      print("\n❌ СБОЙ В РАБОТЕ:")
      traceback.print_exc()
    finally:
      running = False
      set_light(False)
      try:
        picam2.stop()
      except Exception:
        pass
      if bus is not None:
        try:
          bus.shutdown()
        except Exception:
          pass
      print("Скрипт завершил работу.")
      os._exit(0)

if __name__ == "__main__":
    main()