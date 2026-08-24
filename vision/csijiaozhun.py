import cv2
import time
from picamera2 import Picamera2

def nothing(x):
    pass

def main():
    print("正在初始化 CSI 排线摄像头以进行白平衡校准...")
    
    try:
        picam2 = Picamera2()
        config = picam2.create_video_configuration(main={"size": (320, 320)})
        picam2.configure(config)
        picam2.start()
        
        # 强制关闭自动白平衡
        picam2.set_controls({"AwbEnable": False})
        print("自动白平衡已关闭，摄像头启动成功！")
        time.sleep(1)
    except Exception as e:
        print(f"摄像头启动失败: {e}")
        return

    # 创建 GUI 窗口与滑动条
    cv2.namedWindow('White Balance Tracker', cv2.WINDOW_NORMAL)
    cv2.resizeWindow('White Balance Tracker', 400, 150)

    # 增益滑块范围设为 0-800，代表 0.0 - 8.0 倍增益
    cv2.createTrackbar('Red Gain', 'White Balance Tracker', 150, 800, nothing)
    cv2.createTrackbar('Blue Gain', 'White Balance Tracker', 150, 800, nothing)

    print("========================================")
    print(" 🎨 白平衡手动校准工具已启动。")
    print(" 建议操作：将一张纯白色的纸放在摄像头前。")
    print(" 拖动 'Red Gain' 和 'Blue Gain'，直到画面中的白纸看起来最接近纯白色。")
    print(" 调整完毕后按 'q' 退出，终端将输出最终的增益参数。")
    print("========================================")

    while True:
        # 获取滑动条的值，并转换为浮点数
        r_gain_raw = cv2.getTrackbarPos('Red Gain', 'White Balance Tracker')
        b_gain_raw = cv2.getTrackbarPos('Blue Gain', 'White Balance Tracker')
        
        # 避免增益为0导致错误，给个 0.1 的保底值
        r_gain = max(0.1, r_gain_raw / 100.0)
        b_gain = max(0.1, b_gain_raw / 100.0)

        # 实时写入最新的白平衡增益参数
        try:
            picam2.set_controls({"ColourGains": (r_gain, b_gain)})
            
            # 捕获画面并转换色彩空间
            img_rgb = picam2.capture_array()
            frame = cv2.cvtColor(img_rgb, cv2.COLOR_RGB2BGR)
            
            # 在画面上显示当前数值
            cv2.putText(frame, f"Red Gain: {r_gain:.2f}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
            cv2.putText(frame, f"Blue Gain: {b_gain:.2f}", (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 0, 0), 2)
            
            cv2.imshow('Camera View', frame)
            
        except Exception as e:
            print(f"读取或设置画面失败: {e}")
            time.sleep(1)
            continue

        # 按 'q' 退出并打印结果
        if cv2.waitKey(1) & 0xFF == ord('q'):
            print("\n============ 校准结束 ============")
            print("请将以下代码参数替换到你的主程序中：")
            print(f"picam2.set_controls({{\"AwbEnable\": False, \"ColourGains\": ({r_gain:.2f}, {b_gain:.2f})}})")
            print("==================================")
            break

    # 释放资源
    try:
        picam2.stop()
    except:
        pass
    cv2.destroyAllWindows()

if __name__ == '__main__':
    main()