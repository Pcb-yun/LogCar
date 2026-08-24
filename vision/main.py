import numpy as np
import serial
import cv2
import yanse as ys
import yuanhuan as yh
import yuanhuan1 as yh1
import yuanhuan2 as yh2
import time
from threading import Thread
from picamera2 import Picamera2

# 全局变量声明
ser = None
response = None           # 修复：初始化为 None
frame_csi_global = None   # 用于多线程共享排线摄像头画面
ret_csi_global = False

def race():
    global ser, response
    while True:
        try:
            if ser is None or not ser.is_open:
                ser = serial.Serial("/dev/ttyAMA0", 115200, timeout=1)
                time.sleep(2)

            data = ser.read(1)
            if data in [b'\x0A', b'\x0B', b'\x0C', b'\x0E',b'\x10',b'\x11']:
                response = data
                print(f"到达新任务点,收到新的指令{data}")
                ser.reset_input_buffer()
        except Exception as e:
            print("串口异常")
            if ser is not None:
                ser.close()
            time.sleep(1)

def csi_reader(picam2):
    """独立的 CSI 摄像头读取线程，防止阻塞主循环卡顿"""
    global frame_csi_global, ret_csi_global
    while True:
        try:
            img_rgb = picam2.capture_array()
            # 更新全局画面供主线程使用
            frame_csi_global = cv2.cvtColor(img_rgb, cv2.COLOR_RGB2BGR)
            ret_csi_global = True
        except Exception as e:
            ret_csi_global = False
        time.sleep(0.005)

def send_data_to_mcu(data_bytes):
    global ser
    if ser is not None and ser.is_open:
        try:
            ser.write(data_bytes)
        except Exception as e:
            print("发送数据失败")

def main():
    global response, frame_csi_global, ret_csi_global
    print("Beginning")
    response = None#b'\x11'#None
    run_text = ''
    res_text = ''

    yuanhuan_start = b'\x0A'
    yuanhuan1_start=b'\x10'
    yuanhuan2_start=b'\x11'
    yanse_start = b'\x0B'
    yansexiuxian_start = b'\x0C'
    yuanhuanxiuxian_start = b'\x0E'
    
    lst = time.time()

    # 初始化 USB 摄像头 (对应圆环识别)
    cap_usb = cv2.VideoCapture(0, cv2.CAP_V4L2)
    cap_usb.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
    cap_usb.set(cv2.CAP_PROP_FRAME_HEIGHT, 320)
    
    # 初始化 CSI 排线摄像头 (对应颜色识别)
    try:
        picam2 = Picamera2()
        config = picam2.create_video_configuration(main={"size": (320, 320)})
        picam2.configure(config)
        picam2.start()
        picam2.set_controls({"AwbEnable": False, "ColourGains": (1.5, 1.2)})
        print("CSI 排线摄像头启动成功！")
        time.sleep(1)
        # 启动 CSI 独立读取线程
        Thread(target=csi_reader, args=(picam2,), daemon=True).start()
    except Exception as e:
        print(f"CSI 排线摄像头启动失败: {e}")
    
    # 启动串口线程
    Thread(target=race, daemon=True).start()
    
    while True:
        # --- 读取画面 ---
        ret_usb, frame_usb = cap_usb.read()
        
        # 直接从内存获取 CSI 最新画面 (非阻塞)
        ret_csi = ret_csi_global
        if ret_csi and frame_csi_global is not None:
            img_csi = frame_csi_global.copy()
            
        if ret_usb:
            img_usb = frame_usb.copy()
            
        # 计算 FPS
        st = time.time()
        diff = st - lst
        run_t = int(1 / diff) if diff > 0.005 else 0
        lst = st
        
        # --- 任务逻辑分支 ---
        if response == yansexiuxian_start:
            run_text = "yansexiuxian_start"
            res_text = "Idle"
                
        elif response == yuanhuanxiuxian_start:
            run_text = "yuanhuanxiuxian_start"
            res_text = "Idle"
                
        elif response == yanse_start:
            run_text = "yanse_start"
            if ret_csi:
                try:
                    color_id = ys.identify_color(img_csi)
                    res_text = f"Color ID: {color_id}"
                    payload = bytes([int(color_id)])
                    send_data_to_mcu(payload)
                except Exception as e:
                    res_text = "Yanse Error"
                
        elif response == yuanhuan_start:
            run_text = "yuanhuan_start"
            if ret_usb:
                try:
                    result = yh.process_yuanhuan(img_usb)
                    if result[0]:
                        qx0, qy0 = result[0]
                        dx_mm, dy_mm = result[1], result[2]
                        is_stable = result[3]  # 获取新增的“数据不跳动”标志
                        
                        # 只有上下帧数据连续 15 次差别很小（稳定）时，才发给电控
                        if is_stable:
                            dir_x = 1 if dx_mm >= 0 else 0
                            dir_y = 1 if dy_mm >= 0 else 0
                            abs_dx = int(abs(dx_mm))
                            abs_dy = int(abs(dy_mm))
                            print(abs_dx,abs_dy)
                            res_text = f"dX:{dx_mm:.0f} dY:{dy_mm:.0f} [SEND]"
                            payload = bytes([0xFE,dir_x, dir_y, abs_dx & 0xFF, abs_dy & 0xFF])
                            send_data_to_mcu(payload)
                        else:
                            # 数据还在跳动/未稳定，等待
                            res_text = f"dX:{dx_mm:.0f} dY:{dy_mm:.0f} [WAIT]"
                    else:
                        res_text = "No Circle"
                except Exception as e:
                    res_text = "Yuanhuan Error"

        elif response == yuanhuan1_start:
                    run_text = "yuanhuan1_start"
                    if ret_usb:
                        try:
                            result = yh1.process_yuanhuan(img_usb)
                            if result[0]:
                                qx0, qy0 = result[0]
                                dx_mm, dy_mm = result[1], result[2]
                                is_stable = result[3]  # 获取新增的“数据不跳动”标志
                                
                                # 只有上下帧数据连续 15 次差别很小（稳定）时，才发给电控
                                if is_stable:
                                    dir_x = 1 if dx_mm >= 0 else 0
                                    dir_y = 1 if dy_mm >= 0 else 0
                                    abs_dx = int(abs(dx_mm))
                                    abs_dy = int(abs(dy_mm))
                                    print(abs_dx,abs_dy)
                                    res_text = f"dX:{dx_mm:.0f} dY:{dy_mm:.0f} [SEND]"
                                    payload = bytes([0xFE,dir_x, dir_y, abs_dx & 0xFF, abs_dy & 0xFF])
                                    send_data_to_mcu(payload)
                                else:
                                    # 数据还在跳动/未稳定，等待
                                    res_text = f"dX:{dx_mm:.0f} dY:{dy_mm:.0f} [WAIT]"
                            else:
                                res_text = "No Circle"
                        except Exception as e:
                            res_text = "Yuanhuan Error"

        elif response == yuanhuan2_start:
                    run_text = "yuanhuan2_start"
                    if ret_usb:
                        try:
                            result = yh2.process_yuanhuan(img_usb)
                            if result[0]:
                                qx0, qy0 = result[0]
                                dx_mm, dy_mm = result[1], result[2]
                                is_stable = result[3]  # 获取新增的“数据不跳动”标志
                                
                                # 只有上下帧数据连续 15 次差别很小（稳定）时，才发给电控
                                if is_stable:
                                    dir_x = 1 if dx_mm >= 0 else 0
                                    dir_y = 1 if dy_mm >= 0 else 0
                                    abs_dx = int(abs(dx_mm))
                                    abs_dy = int(abs(dy_mm))
                                    print(abs_dx,abs_dy)
                                    res_text = f"dX:{dx_mm:.0f} dY:{dy_mm:.0f} [SEND]"
                                    payload = bytes([0xFE,dir_x, dir_y, abs_dx & 0xFF, abs_dy & 0xFF])
                                    send_data_to_mcu(payload)
                                else:
                                    # 数据还在跳动/未稳定，等待
                                    res_text = f"dX:{dx_mm:.0f} dY:{dy_mm:.0f} [WAIT]"
                            else:
                                res_text = "No Circle"
                        except Exception as e:
                            res_text = "Yuanhuan Error"
                
        elif response is None:
            run_text = "Wait Command"
            res_text = "Idle"

        # --- 画面绘制与显示 ---
        if ret_csi:
            cv2.putText(img_csi, f"RUN:{run_text}", (0, 25), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 255), 2)
            cv2.putText(img_csi, f"RES:{res_text}", (0, 52), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 255), 2)
            cv2.putText(img_csi, f"FPS:{run_t}", (0, 79), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 255), 2)
            cv2.imshow('CSI (Yanse)', img_csi)
            
        if ret_usb:
            cv2.putText(img_usb, f"RUN:{run_text}", (0, 25), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 255), 2)
            cv2.putText(img_usb, f"RES:{res_text}", (0, 52), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 255), 2)
            cv2.putText(img_usb, f"FPS:{run_t}", (0, 79), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 255), 2)
            cv2.imshow('USB (Yuanhuan)', img_usb)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
            
    # 安全释放
    try:
        picam2.stop()
    except:
        pass
    cap_usb.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
