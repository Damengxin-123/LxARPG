"""将虚幻引擎实际离屏渲染结果整理为带中文标签的技能预览总图。"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


def make_contact_sheet():
    """按角色光环、弹体和爆裂排列实际截图，不重绘或替换引擎输出内容。"""
    project = Path(__file__).resolve().parents[3]
    output = project / "Saved" / "技能美术"
    render = output / "渲染"
    entries = [
        ("火之领域 · 身体与头顶环", "火之领域_角色.png"),
        ("衰弱领域 · 紫晶头顶环", "衰弱领域_角色.png"),
        ("光环测试 · 碧蓝头顶环", "光环测试_角色.png"),
        ("火系 · 绯焰弹", "绯焰弹_38.png"),
        ("冰系 · 霜晶弹", "霜晶弹_38.png"),
        ("咒术 · 咒晶飞弹", "咒晶飞弹_38.png"),
        ("火系 · 绯焰爆裂", "绯焰爆裂_14.png"),
        ("冰系 · 寒晶碎裂", "寒晶碎裂_14.png"),
        ("咒术 · 咒印扩散", "咒印扩散_38.png"),
    ]
    canvas = Image.new("RGB", (1260, 1130), (16, 23, 33))
    draw = ImageDraw.Draw(canvas)
    title = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 34)
    label = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 22)
    small = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 19)
    draw.text((30, 20), "LxARPG  技能美术预览", font=title, fill=(224, 236, 246))
    draw.text((30, 68), "UE 5.8 实际渲染  ·  发光半透明符文环 / 分层粒子 / 原创模型", font=small, fill=(143, 169, 189))
    for index, (name, filename) in enumerate(entries):
        col, row = index % 3, index // 3
        x, y = 24 + col * 412, 108 + row * 330
        draw.rounded_rectangle((x, y, x + 396, y + 318), radius=10, fill=(26, 37, 49), outline=(59, 78, 92), width=1)
        image = Image.open(render / filename).convert("RGB").resize((384, 277), Image.Resampling.LANCZOS)
        canvas.paste(image, (x + 6, y + 6))
        draw.text((x + 12, y + 287), name, font=label, fill=(206, 223, 236))
    canvas.save(output / "技能预览.png")
    print(output / "技能预览.png")


def make_refinement_comparison():
    """以同机位、同播放时间的引擎截图对照原版与精修版。"""
    project = Path(__file__).resolve().parents[3]
    output = project / "Saved" / "技能美术"
    before = output / "精修前备份" / "Saved" / "技能美术" / "渲染"
    after = output / "渲染"
    canvas = Image.new("RGB", (1260, 1470), (16, 23, 33))
    draw = ImageDraw.Draw(canvas)
    title = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 34)
    label = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 22)
    small = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 19)
    draw.text((30, 18), "LxARPG  技能精修前后对照", font=title, fill=(224, 236, 246))
    draw.text((30, 65), "相同机位与播放时间 · UE 5.8 实际渲染", font=small, fill=(143, 169, 189))
    rows = [
        (before, "原版", [("绯焰弹", "绯焰弹_38.png"), ("霜晶弹", "霜晶弹_38.png"), ("咒晶飞弹", "咒晶飞弹_38.png")]),
        (after, "精修", [("绯焰弹", "绯焰弹_38.png"), ("霜晶弹", "霜晶弹_38.png"), ("咒晶飞弹", "咒晶飞弹_38.png")]),
        (before, "原版", [("火之领域", "火之领域_角色.png"), ("绯焰爆裂", "绯焰爆裂_14.png"), ("寒晶碎裂", "寒晶碎裂_14.png")]),
        (after, "精修", [("火之领域", "火之领域_角色.png"), ("绯焰爆裂", "绯焰爆裂_14.png"), ("寒晶碎裂", "寒晶碎裂_14.png")]),
    ]
    for row, (directory, version, entries) in enumerate(rows):
        for col, (name, filename) in enumerate(entries):
            x, y = 24 + col * 412, 108 + row * 332
            draw.rounded_rectangle((x, y, x + 396, y + 318), radius=10, fill=(26, 37, 49), outline=(59, 78, 92), width=1)
            image = Image.open(directory / filename).convert("RGB").resize((384, 277), Image.Resampling.LANCZOS)
            canvas.paste(image, (x + 6, y + 6))
            draw.text((x + 12, y + 287), version + " · " + name, font=label, fill=(206, 223, 236))
    canvas.save(output / "精修对照.png")
    print(output / "精修对照.png")


if __name__ == "__main__":
    make_contact_sheet()
    if (Path(__file__).resolve().parents[3] / "Saved" / "技能美术" / "精修报告.txt").exists():
        make_refinement_comparison()
