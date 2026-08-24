import cv2
import numpy as np
import time
from picamera2 import Picamera2

def nothing(x):
    pass

def main():
    print("正在初始化 CSI 排线摄像头...")
    
    # 1. 初始化排线摄像头 (与你 main.py 中保持一致)
    try:
        picam2 = Picamera2()
        config = picam2.create_video_configuration(main={"size": (320, 320)})
        picam2.configure(config)
        picam2.start()
        print("CSI 排线摄像头启动成功！")
        time.sleep(1) # 等待预热
    except Exception as e:
        print(f"CSI 排线摄像头启动失败: {e}")
        return

    # 2. 创建 GUI 窗口与滑动条
    cv2.namedWindow('Trackbars', cv2.WINDOW_NORMAL)
    cv2.resizeWindow('Trackbars', 400, 300)

    # 创建 H, S, V 的上限和下限滑动条
    # OpenCV 中 H 的范围是 0-180，S 和 V 的范围是 0-255
    cv2.createTrackbar('H_min', 'Trackbars', 0, 180, nothing)
    cv2.createTrackbar('H_max', 'Trackbars', 180, 180, nothing)
    cv2.createTrackbar('S_min', 'Trackbars', 0, 255, nothing)
    cv2.createTrackbar('S_max', 'Trackbars', 255, 255, nothing)
    cv2.createTrackbar('V_min', 'Trackbars', 0, 255, nothing)
    cv2.createTrackbar('V_max', 'Trackbars', 255, 255, nothing)

    print("========================================")
    print(" 颜色阈值测试工具已启动。")
    print(" 请在弹出的 'Trackbars' 窗口中拖动滑块调整范围。")
    print(" 观察 'Mask' 窗口，纯白色的区域代表被成功识别出来的部分。")
    print(" 调整完毕后，请按键盘上的 'q' 键退出，控制台会输出你的最终阈值。")
    print("========================================")

    while True:
        # 3. 读取排线摄像头画面
        try:
            img_rgb = picam2.capture_array()
            # 必须转换为 BGR 才能在 OpenCV 中正常显示和转换颜色空间
            frame = cv2.cvtColor(img_rgb, cv2.COLOR_RGB2BGR)
        except Exception as e:
            print(f"读取画面失败: {e}")
            time.sleep(1)
            continue

        # 4. 转换到 HSV 色彩空间进行处理
        # 先做个高斯模糊，减少画面噪点对提取颜色的干扰
        blurred = cv2.GaussianBlur(frame, (5, 5), 0)
        hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

        # 5. 获取滑动条的当前实时值
        h_min = cv2.getTrackbarPos('H_min', 'Trackbars')
        h_max = cv2.getTrackbarPos('H_max', 'Trackbars')
        s_min = cv2.getTrackbarPos('S_min', 'Trackbars')
        s_max = cv2.getTrackbarPos('S_max', 'Trackbars')
        v_min = cv2.getTrackbarPos('V_min', 'Trackbars')
        v_max = cv2.getTrackbarPos('V_max', 'Trackbars')

        # 6. 设置阈值并生成二值化掩膜 (Mask)
        lower_bound = np.array([h_min, s_min, v_min])
        upper_bound = np.array([h_max, s_max, v_max])
        mask = cv2.inRange(hsv, lower_bound, upper_bound)

        # 7. 将掩膜与原图进行位运算 (仅保留掩膜为白色的彩色区域)
        result = cv2.bitwise_and(frame, frame, mask=mask)

        # 8. 在原图上加上文字，方便直接在画面上看当前数值
        cv2.putText(frame, f"Lower: [{h_min}, {s_min}, {v_min}]", (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
        cv2.putText(frame, f"Upper: [{h_max}, {s_max}, {v_max}]", (10, 55), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

        # 显示三个窗口
        cv2.imshow('Original (Camera)', frame)
        cv2.imshow('Mask (White = Detected)', mask)
        cv2.imshow('Result (Filtered)', result)

        # 9. 按 'q' 退出并打印最终阈值
        if cv2.waitKey(1) & 0xFF == ord('q'):
            print("============ 测试结束 ============")
            print("请将以下代码复制到你的 yanse.py 中：")
            print(f"lower_target = np.array([{h_min}, {s_min}, {v_min}])")
            print(f"upper_target = np.array([{h_max}, {s_max}, {v_max}])")
            print("==================================")
            break

    # 安全释放资源
    try:
        picam2.stop()
    except:
        pass
    cv2.destroyAllWindows()

if __name__ == '__main__':
    main()
