import cv2
import numpy as np

def identify_color(img):
    if img is None or img.size == 0:
        return 0
    blurred = cv2.GaussianBlur(img, (5, 5), 0)
    hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)
    # 定义各颜色的 HSV 阈值范围 
    # 红色
    lower_red1 = np.array([0, 70, 50])
    upper_red1 = np.array([10, 255, 255])
    lower_red2 = np.array([160, 70, 50])
    upper_red2 = np.array([180, 255, 255])
    # 绿色
    lower_green = np.array([35, 70, 50])
    upper_green = np.array([85, 255, 255])
    # 蓝色
    lower_blue = np.array([100, 70, 50])
    upper_blue = np.array([130, 255, 255])
    # 白色
    lower_white = np.array([0, 0, 180])
    upper_white = np.array([180, 40, 255])
    # 黑色
    lower_black = np.array([0, 0, 0])
    upper_black = np.array([180, 255, 50])
    mask_red1 = cv2.inRange(hsv, lower_red1, upper_red1)
    mask_red2 = cv2.inRange(hsv, lower_red2, upper_red2)
    mask_red = cv2.bitwise_or(mask_red1, mask_red2) # 拼接红色掩膜
    mask_green = cv2.inRange(hsv, lower_green, upper_green)
    mask_blue = cv2.inRange(hsv, lower_blue, upper_blue)
    mask_white = cv2.inRange(hsv, lower_white, upper_white)
    mask_black = cv2.inRange(hsv, lower_black, upper_black)
    
    # 统计每种颜色的像素点数量
    color_counts = {
        1: cv2.countNonZero(mask_red),
        2: cv2.countNonZero(mask_green),
        3: cv2.countNonZero(mask_blue),
        4: cv2.countNonZero(mask_white),
        5: cv2.countNonZero(mask_black)
    }
    
    # 找出像素最多的颜色
    dominant_color_id = max(color_counts, key=color_counts.get)
    
    if color_counts[dominant_color_id] < 100:
        return 0
    
    return dominant_color_id

# 测试
if __name__ == "__main__":
    # 打开摄像头测试
    cap = cv2.VideoCapture(0)
    
    while True:
        ret, frame = cap.read()
        if not ret:
            break
            
        # 建议取画面中间的一小块区域 (ROI) 来识别颜色，避免受背景干扰
        height, width = frame.shape[:2]
        cx, cy = width // 2, height // 2
        box_size = 50  # 截取中心 100x100 的矩形区域
        
        roi = frame[cy-box_size : cy+box_size, cx-box_size : cx+box_size]
        
        # 调用颜色识别模块
        color_id = identify_color(roi)
        
        # 在画面上绘制识别区域和结果
        cv2.rectangle(frame, (cx-box_size, cy-box_size), (cx+box_size, cy+box_size), (0, 255, 255), 2)
        cv2.putText(frame, f"Color ID: {color_id}", (cx - box_size, cy - box_size - 10), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
                    
        cv2.imshow("Color Recognition", frame)
        cv2.imshow("ROI", roi)
        
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
            
    cap.release()
    cv2.destroyAllWindows()