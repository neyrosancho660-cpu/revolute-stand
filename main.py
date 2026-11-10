import re
import enum
import math
import bisect
import asyncio
import argparse
import datetime
import numpy as np
import matplotlib.pyplot as plt

from rtt_logger import AsyncRTT


class ReaderType(enum.StrEnum):
    iRayple_RS485 = 'irayple_rs485'
    iRayple_CANOpen = 'irayple_canopen'
    Pepperl_Fuchs_CANOpen = 'pepperl_fuchs_canopen'


def parse(log_text):
    """
    Parsing
    """
    marker_id = re.search(r"Marker ID:\s*(\d+)", log_text)
    target_speed = re.search(r"Speed:\s*([\d.]+)", log_text)
    motor_offset = re.search(r"Motor offset:\s*([-]?\d+)", log_text)
    reader_offset = re.search(r"PGV offset:\s*([-]?\d+)", log_text)

    if not all([marker_id, target_speed, motor_offset, reader_offset]):
        return None

    target_id = int(marker_id.group(1))                 # Target makrer ID
    target_speed = float(target_speed.group(1))         # Target speed in RPM
    motor_offset = int(motor_offset.group(1))           # Motor angle offset in millidegrees
    reader_offset = int(reader_offset.group(1))         # Reader angle offset in millidegrees

    motor_reader_offset = 0                             # Additional offset to align motor and reader angles (calculated later)    
    reached_terget_speed_time = 0                       # Time when motor reaches target speed (calculated later)


    latency = list()                                    # List of dicts with keys "reader_time", "motor_time", "delay_sec"
    motor_result = list()                               # Motor log results: list of dicts with keys "time", "speed", "position"
    reader_result = list()                              # Reader log results: list of dicts with keys "time", "angle"
    counts_per_turn = dict()                            # Dictionary to count how many reader detections per full turn of the motor (key: turn number, value: count)
    reader_response_times = list()                      # List of all reader response times (for frequency calculation)


    re_motor = re.compile(r"Time:\s*(\d+)\s*\|\s*Speed:\s*(-?\d+)\s*RPM\s*\|\s*Position:\s*(-?\d+)")
    re_reader = re.compile(r"Time:\s*(\d+)\s*\|\s*ID:\s*(\d+)\s*\|\s*Angle:\s*(-?\d+)")


    lines = log_text.splitlines()
    for line in log_text.splitlines():
        if "Start" in line: break
    lines = lines[lines.index(line):]


    for line in lines:
        motore_match = re_motor.search(line)
        if motore_match:
            motor_angle = (int(motore_match.group(3)) - motor_offset) / 1000.0
            if motor_angle >= 270.0:
                motor_offset += motor_angle * 1000
                motor_reader_offset = 360.0 - motor_angle
                break
    lines = lines[lines.index(line):]



    for line in lines:

        if "Stop" in line: 
            break

        motore_match = re_motor.search(line)
        if motore_match:
            time = int(motore_match.group(1)) / 1000.0                         
            speed = int(motore_match.group(2)) / 1000.0
            position = (int(motore_match.group(3)) - motor_offset) / 1000.0

            motor_result.append({"time": time, "speed": speed, "position": position})
            
            if reached_terget_speed_time == 0 and speed >= target_speed * 0.95:
                reached_terget_speed_time = time


        reader_match = re_reader.search(line)
        if reader_match:
            id = int(reader_match.group(2))
            time = int(reader_match.group(1)) / 1000.0
            reader_response_times.append(time)

            if id == target_id and int(reader_match.group(3)) != 0:
                turn = int(motor_result[-1]['position'] / 360.0)
                angle = (int(reader_match.group(3)) - reader_offset) / 1000.0 + turn * 360.0 + motor_reader_offset

                reader_result.append({"time": time, "angle": angle})
                if (turn not in counts_per_turn.keys()) or (angle != reader_result[-2]['angle']):
                    counts_per_turn[turn] = counts_per_turn.get(turn, 0) + 1
                    if counts_per_turn[turn] > 13:
                        pass


    for i in range(max(counts_per_turn.keys())):
        if i not in counts_per_turn.keys():
            counts_per_turn[i] = 0

    
    motor_times = [m['time'] for m in motor_result]
    motor_positions = [m['position'] for m in motor_result]

    for r in reader_result:
        if r['time'] < reached_terget_speed_time: continue

        idx = bisect.bisect_left(motor_positions, r['angle'])
        # print("Reader angle: {:.2f}, Motor angle at idx: {:.2f}, Difference: {:<10.2f}, Time: {:.2f}".format(r['angle'], motor_positions[idx] if idx < len(motor_positions) else float('nan'), r['angle'] - motor_positions[idx] if idx < len(motor_positions) else float('nan'), r['time']))
        if 0 < idx < len(motor_positions):

            dist_before = abs(motor_positions[idx - 1] - r['angle'])
            dist_after = abs(motor_positions[idx] - r['angle'])
            
            if dist_before <= dist_after:
                closest_m_time = motor_times[idx-1]
            else:
                closest_m_time = motor_times[idx]
                
            delay = r['time'] - closest_m_time
            latency.append({"delay_sec": delay})
            if delay*1000 > 100:
                pass

    return motor_result, reader_result, reader_response_times, latency, counts_per_turn


def plot_full_dashboard(motor_data, target_reader, all_reader_times, latency_results, counts_per_turn, target_speed, frequency, reader_type, file_name):

    fig = plt.figure(figsize=(16, 14))
    
    gs = fig.add_gridspec(3, 2, height_ratios=[1, 1, 0.5], hspace=0.4, wspace=0.2)
    fig.suptitle(f'{reader_type} {target_speed} m/s', fontsize=20, fontweight='bold', y=0.96)


    test_duration = (motor_data[-1]['time'] - motor_data[0]['time']) if motor_data else 0
    motor_turns = motor_data[-1]['position'] / 360.0 if motor_data else 0
    reader_turns = max(counts_per_turn.keys()) if counts_per_turn else 0
    avg_hits = np.mean(list(counts_per_turn.values())) if counts_per_turn else 0


    num_missed = [key for key, value in counts_per_turn.items() if value == 0]
    if num_missed:
        print(f"Пропущено кругов: {num_missed}")
    num_missed = len(num_missed)

    if latency_results:
        delays_ms = [d['delay_sec'] * 1000 for d in latency_results]
        avg_lat, med_lat, std, max_lat = np.mean(delays_ms), np.median(delays_ms), np.std(delays_ms), np.max(delays_ms)
    else:
        avg_lat = med_lat = std = max_lat = 0

    if len(all_reader_times) > 1:
        resp_intervals = np.diff(all_reader_times) * 1000
        avg_resp_time, max_resp_time = np.mean(resp_intervals), np.max(resp_intervals)
    else:
        avg_resp_time = max_resp_time = 0


    ax1 = fig.add_subplot(gs[0, 0])
    error_deg = []
    if latency_results:
        for i in target_reader:
            time = i['time']
            angle = i['angle']

            for m in motor_data:
                if m['time'] >= time:
                    error = abs(m['position'] - angle)
                    error_deg.append(error)
                    break

        r_times = [r['time'] - target_reader[0]['time'] for r in target_reader]
        ax1.scatter(r_times[:len(error_deg)], error_deg, color='red', s=2, alpha=0.5)
        ax1.set_title('Ошибка положения (Motor - Reader), град.')
        ax1.set_ylabel('Degrees')
        ax1.grid(True, alpha=0.3)


    ax2 = fig.add_subplot(gs[0, 1])
    if len(all_reader_times) > 1:
        ax2.hist(resp_intervals, bins=100, color='mediumpurple', alpha=0.7)
        ax2.set_title('Интервалы между пакетами ридера (мс)')
        ax2.set_xlabel('ms')
        ax2.grid(True, alpha=0.2)


    ax3 = fig.add_subplot(gs[1, 0])
    if latency_results:
        ax3.hist(delays_ms, bins=50, color='skyblue', edgecolor='black', alpha=0.7)
        ax3.set_title(f'Гистограмма задержки (Latency)\nAvg: {avg_lat:.2f} ms')
        ax3.set_xlabel('ms')
        ax3.grid(True, alpha=0.2)


    ax4 = fig.add_subplot(gs[1, 1])
    if counts_per_turn:
        counts = list(counts_per_turn.values())
        bins = np.arange(min(counts)-0.5, max(counts)+1.5, 1)
        ax4.hist(counts, bins=bins, color='salmon', edgecolor='black', rwidth=0.8)
        ax4.set_xticks(range(int(min(counts)), int(max(counts)) + 1))
        ax4.set_title('Распределение детекций за один круг')
        ax4.set_xlabel('Кол-во пакетов в круге')
        ax4.set_ylabel('Кол-во кругов')
        ax4.grid(True, axis='y', alpha=0.2)


    ax_text = fig.add_subplot(gs[2, :])
    ax_text.axis('off')

    
    L_LABEL = 22 
    L_VAL   = 15  
    L_UNIT  = 5   
    GAP     = " " * 10 

    def fmt_line(label, val, unit):
        return f"{label:<{L_LABEL}} {val:>{L_VAL}} {unit:<{L_UNIT}}"

    left_side = [
        f"{'--- КОНФИГУРАЦИЯ ---':<{L_LABEL + L_VAL + L_UNIT}}",
        fmt_line("Тип ридера:", reader_type, ""),
        fmt_line("Cкорость:", f"{target_speed}", "м/c"),
        fmt_line("Время теста:", f"{test_duration:.1f}", "сек"),
        "",
        f"{'--- ОДОМЕТРИЯ ---':<{L_LABEL + L_VAL + L_UNIT}}",
        fmt_line("Кругов (Мотор):", f"{motor_turns:.2f}", ""),
        fmt_line("Кругов (Ридер):", f"{reader_turns}", ""),
        fmt_line("Пропущено кругов:", f"{num_missed}", "шт")
    ]

    right_side = [
        f"{'--- ИНТЕРВАЛЫ ПАКЕТОВ ---':<{L_LABEL + L_VAL + L_UNIT}}",
        fmt_line("Период опроса:", f"{frequency}", "мс"),
        fmt_line("Ср. время ответа:", f"{avg_resp_time:.2f}", "мс"),
        fmt_line("Макс. время ответа:", f"{max_resp_time:.2f}", "мс"),
        "",
        f"{'--- ЗАДЕРЖКА (LATENCY) ---':<{L_LABEL + L_VAL + L_UNIT}}",
        fmt_line("Средняя задержка:", f"{avg_lat:.2f}", "мс"),
        fmt_line("Медианная задержка:", f"{med_lat:.2f}", "мс"),
        fmt_line("Дрожание (std):", f"{std:.2f}", "мс"),
    ]

    final_rows = [f"{l}{GAP}{r}" for l, r in zip(left_side, right_side)]
    final_string = "\n".join(final_rows)

    ax_text.text(0.5, 0.4, final_string, 
                 fontsize=11, 
                 fontfamily='monospace', 
                 va='center', ha='center',
                 linespacing=1.6,
                 bbox=dict(facecolor='#F8F9FA', edgecolor='#D1D5DB', boxstyle='round,pad=1.5', alpha=0.9))

    plt.savefig(f'{file_name}.png', dpi=300, bbox_inches='tight')
    plt.show()


async def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("--id", type=int, default=1057)                     # Target tag ID
    parser.add_argument("--time", "--t", type=int, default=10)              # Test time
    parser.add_argument("--speed", "--s", type=float, default=2)            # Test speed m/s

    parser.add_argument("--angle", "--a", type=float, default=45.0)         # Tag zero angle
    parser.add_argument("--reader", type=str, default='irayple_canopen')    # Reader type
    parser.add_argument("--reader_id", type=int, default=16)                # Reader CANOpen ID
    parser.add_argument("--frequency", "--f", type=float, default=20.0)     # Reader polling frequency 

    parser.add_argument("--motor_id", type=int, default=1)                  # Motor CANOpen ID


    args = parser.parse_args()


    r = 0.115                                                               # Distance between the centers of the marker and the disk
    angular_speed = args.speed * 60 / (2 * math.pi * r)                     # Converition from linear speed to angular speed


    logger = AsyncRTT(chip_name="STM32F103RE")
    if not await logger.connect(): 
        print('RTT connection failed')
        return

    await logger.write(f"kinco connect\n")                                  # Kinco connection
    await logger.write(f"kinco set acceleration {3.0}\n")                   # Kinco acceleration setter
    await logger.write(f"kinco set decceleration {3.0}\n")                  # Kinco deceleration setter
    await logger.write(f"kinco set profile_speed {1.0}\n")                  # Kinco speed setter

    await logger.write(f"pgv type {args.reader}\n")                         # PGV type setter
    if args.reader == ReaderType.iRayple_RS485:
        frequency = args.frequency                             
        await logger.write(f"pgv frequency {frequency}\n")                  # Irayple RS485 polling frequency setter
    elif args.reader in (ReaderType.iRayple_CANOpen, ReaderType.Pepperl_Fuchs_CANOpen):
        frequency = '-'
        await logger.write(f"pgv id {args.reader_id}\n")                    # Irayple CANOpen ID setter

    await logger.write(f"test angle {args.angle}\n")                        # PGV zero angle setter
    await logger.write(f"test speed {angular_speed}\n")                     # Test angular speed setter
    await logger.write(f"test time {args.time * 1000}\n")                   # Test time setter
    await logger.write(f"test id {args.id}\n")                              # Test target marker ID setter
    await logger.write(f"test start\n")                                     # Start test

    
    config_header = (                                                       # File header
        f"--- Launch configuration ---\n"
        f"Time: {args.time} s\n"
        f"Marker ID: {args.id}\n"
        f"Speed: {angular_speed} rpm\n"
    )
    print(config_header)

    full_log = config_header
    file_name = f"test_log_{args.speed}_{args.reader}_{args.frequency}_{datetime.datetime.now().strftime('%Y_%m_%d_%H_%M')}"
    
    async for log in logger.read_stream(polling_interval=0.00):
        full_log += log
        if "Stop" in full_log: break

    with open(file_name + '.txt', "w") as f:
        f.write(full_log)

    print("Analyzing data...")
    motor_result, reader_result, reader_response_times, latency, counts_per_turn = parse(full_log)
    
    if motor_result and reader_response_times:
        plot_full_dashboard(
            motor_result,
            reader_result, 
            reader_response_times, 
            latency, 
            counts_per_turn, 
            args.speed, 
            frequency, 
            args.reader,
            file_name
            )
    else:
        print("Error: No data to plot.")


if __name__ == "__main__":
    asyncio.run(main())