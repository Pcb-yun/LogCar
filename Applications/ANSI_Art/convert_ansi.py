#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# 该脚本将 ANSI 文件中的每个字符转换为 C 语言可用的转义序列
# 并将结果写入新的文件中。
# 使用方法: python convert_ansi.py <输入文件> <输出文件>

import sys
import os

def escape_char(c):
    """将字符转换为 C 语言转义序列"""
    # ESC 字符转换为 \033
    if c == '\x1b':
        return '\\033'

    # 普通可打印 ASCII 字符直接输出
    if 32 <= ord(c) <= 126:
        return c

    # 其他字符转换为 UTF-8 字节序列
    utf8_bytes = c.encode('utf-8')
    return ''.join(f'\\x{byte:02x}' for byte in utf8_bytes)

def convert_file(input_path, output_path):
    """将 ANSI 文件转换为 C 语言可用的格式"""
    try:
        # 读取原始文件（UTF-8 编码）
        with open(input_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # 逐字符转换
        converted_lines = []
        for line in content.split('\n'):
            escaped_line = ''.join(escape_char(c) for c in line)
            converted_lines.append(escaped_line)

        # 写入输出文件
        with open(output_path, 'w', encoding='utf-8') as f:
            for i, line in enumerate(converted_lines):
                # 跳过空行
                if not line:
                    continue
                if i < len(converted_lines) - 1:
                    f.write('\t"' + line + '\\r\\n"\n')
                else:
                    f.write('\t"' + line + '"')

        print(f"转换完成！输入文件: {input_path}")
        print(f"输出文件: {output_path}")
        print(f"共转换 {len(converted_lines)} 行")

    except Exception as e:
        print(f"转换失败: {e}", file=sys.stderr)
        sys.exit(1)

def main():
    if len(sys.argv) != 3:
        print("用法: python convert_ansi.py <输入文件> <输出文件>")
        print("示例: python convert_ansi.py output.txt output_c.txt")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    if not os.path.exists(input_path):
        print(f"错误: 输入文件不存在 - {input_path}", file=sys.stderr)
        sys.exit(1)

    convert_file(input_path, output_path)

if __name__ == '__main__':
    main()
