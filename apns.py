import xml.etree.ElementTree as ET
import json

# 解析 XML 文件
tree = ET.parse('apns-conf.xml')
root = tree.getroot()

# 用于存储提取的APN信息的列表
apn_list = []

# 用于跟踪唯一的 code 值
seen_codes = set()

# 遍历所有 <apn> 标签
for apn in root.findall('apn'):
    carrier = apn.get('carrier')
    mcc = apn.get('mcc')
    mnc = apn.get('mnc')
    apn_name = apn.get('apn')
    
    # 跳过apn属性为空的节点
    if not apn_name:
        continue
    
    # 生成 code 值
    code = f"{mcc}{mnc}"
    
    # 检查 code 是否已存在
    if code not in seen_codes:
        # 如果 code 是唯一的，则添加到列表和集合中
        apn_data = {
            "name": carrier,
            "code": code,
            "apn": apn_name
        }
        apn_list.append(apn_data)
        seen_codes.add(code)

# 将列表转换为单行 JSON 字符串
json_data = json.dumps(apn_list, separators=(',', ':'))
json_data = json_data.replace('},{', '},\n{')
# 将 JSON 数据写入文件
with open('apns.json', 'w') as json_file:
    json_file.write(json_data)

print("APN数据已转换为JSON并保存到apns.json文件中。")
