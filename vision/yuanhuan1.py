import cv2
import numpy as np

# 像素转毫米
MM_PER_PIXEL = 0.38  # 1像素=1mm[cite: 5]   0.46

# 卡尔曼滤波初始化 (清理了冗余无用变量)
kalman = cv2.KalmanFilter(2, 2)
kalman.measurementMatrix = np.array([[1, 0], [0, 1]], np.float32)
kalman.transitionMatrix = np.array([[1, 0], [0, 1]], np.float32)
kalman.processNoiseCov = np.array([[1, 0], [0, 1]], np.float32) * 1e-3
kalman.measurementNoiseCov = np.array([[1, 0], [0, 1]], np.float32) * 0.01
kalman.statePre = np.array([[6], [6]], np.float32)

#上下帧对比稳定性判定变量 
stable_count = 0        # 连续稳定计数器
prev_qx0 = None         # 上一帧的 X 坐标
prev_qy0 = None         # 上一帧的 Y 坐标
STABLE_FRAMES = 15      # 目标连续稳定的帧数要求
DIFF_THRESHOLD = 5      # 帧间变化阈值(像素或mm)：上下两帧坐标差值小于此值算作"偏差很小"
# ========

def kalman_filter(measured_value):
    global kalman
    # 更新测量值并直接预测
    kalman.correct(measured_value)
    current_prediction = kalman.predict()
    return current_prediction

# 圆环识别
def circle_position(img):
    # 直接转换为单通道灰度图
    gray_img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    
    # 高斯模糊去噪 
    blurred = cv2.GaussianBlur(gray_img, (7, 7), 2)
    
    # CLAHE 对比度受限自适应直方图均衡化 
    clahe = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(8, 8))
    clahed_img = clahe.apply(blurred)

    # 霍夫圆检测 
    circles = cv2.HoughCircles(
        clahed_img, 
        cv2.HOUGH_GRADIENT_ALT, 
        dp=1.5, 
        minDist=50, 
        param1=300, 
        param2=0.8,   # 降低完美度要求，容忍椭圆畸变
        minRadius=30, 
        maxRadius=120 # 扩大半径上限，适应小车靠近目标
    )

    if circles is not None:
        circles = np.uint16(np.around(circles))
        sum_x, sum_y, count = 0, 0, 0
        
        # 遍历所有找到的圆环
        for circle in circles[0, :]:
            cv2.circle(img, (circle[0], circle[1]), circle[2], (0, 0, 255), 2)
            cv2.circle(img, (circle[0], circle[1]), 2, (255, 0, 0), 2)
            sum_x += circle[0]
            sum_y += circle[1]
            count += 1
        
        # 返回平均中心点
        return int(sum_x / count), int(sum_y / count)
        
    return None

# 调用的综合处理函数
def process_yuanhuan(img):
    global stable_count, prev_qx0, prev_qy0
    
    height, width = img.shape[:2]
    vis_cx = width // 2
    vis_cy = height // 2

    # 圆环定位
    target_center = circle_position(img)

    if target_center:
        measured_x, measured_y = target_center
        
        # 卡尔曼滤波处理
        z = np.array([[measured_x], [measured_y]], dtype=np.float32)
        x = kalman_filter(z)
        qx0 = int(x[0][0])
        qy0 = int(x[1][0])

        # 上下两帧数据对比逻辑
        if prev_qx0 is not None and prev_qy0 is not None:
            # 计算当前帧与上一帧的坐标变化量
            delta_x = abs(qx0 - prev_qx0)
            delta_y = abs(qy0 - prev_qy0)
            
            # 如果变化量小于设定的阈值，说明数据稳定
            if delta_x < DIFF_THRESHOLD and delta_y < DIFF_THRESHOLD:
                stable_count += 1
            else:
                # 变化量过大，说明在移动或晃动，重新计数
                stable_count = 0
        else:
            # 第一帧检测到，无法对比，重置为 0
            stable_count = 0
            
        # 更新上一帧的数据，供下一帧对比使用
        prev_qx0 = qx0
        prev_qy0 = qy0
        
        # 判断是否连续 15 帧稳定
        is_stable = (stable_count >= STABLE_FRAMES)

        # 距离计算
        dx_pixel = qx0 - vis_cx
        dy_pixel = vis_cy - qy0 

        # 像素转毫米
        dx_mm = dx_pixel * MM_PER_PIXEL#+16
        dy_mm = dy_pixel * MM_PER_PIXEL#-1
        
        # 在图像上绘制辅助线和数据
        cv2.line(img, (vis_cx, vis_cy), (qx0, qy0), (255, 0, 255), 2)
        cv2.circle(img, (vis_cx, vis_cy), 5, (0, 255, 0), -1)
        
        # 稳定时字体变为绿色，未稳定时为黄色
        text_color = (0, 255, 0) if is_stable else (0, 255, 255)
        cv2.putText(img, f"dX:{dx_mm:.0f} dY:{dy_mm:.0f} CNT:{stable_count}", (qx0 + 10, qy0 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, text_color, 2)
        
        # 返回新增第四个元素: is_stable 标志
        return (qx0, qy0), dx_mm, dy_mm, is_stable

    # 如果当前帧没有检测到圆环，清理历史数据和计数器
    stable_count = 0
    prev_qx0 = None
    prev_qy0 = None
    return None, 0, 0, False

# 保留用于单独测试的代码
if __name__ == "__main__":
    cap = cv2.VideoCapture(0)
    while True:
        success, img0 = cap.read()
        if success:
            process_yuanhuan(img0)
            cv2.imshow("Test USB", img0)
        else:
            break
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    cap.release()
    cv2.destroyAllWindows()
