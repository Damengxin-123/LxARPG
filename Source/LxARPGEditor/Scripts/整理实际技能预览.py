"""排列真实技能单元的引擎截图；保留模型和粒子实际渲染的像素内容。"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


def make_runtime_sheet():
    """将正式流程的模型、拖尾、爆炸、射线及光环整理为中文预览图。"""
    project = Path(__file__).resolve().parents[3]
    output = project / "Saved" / "技能美术"
    render = output / "实际技能渲染"
    entries = [
        ("火球术 · 球体与火焰拖尾", "火球术_节点02_0.65秒.png"),
        ("冰锥术 · 晶体与寒星", "冰锥术_节点02_0.65秒.png"),
        ("厄运诅咒 · 咒晶与紫光", "厄运诅咒_节点02_0.65秒.png"),
        ("火球术 · 命中爆裂", "火球术_节点03_0.25秒.png"),
        ("火焰喷射 · 连续火焰", "火焰喷射_节点02_0.25秒.png"),
        ("火焰射线 · 瞬发光束", "火焰射线_节点02_0.10秒.png"),
        ("火之领域 · 身体符文环", "火之领域_节点03_0.65秒.png"),
        ("衰弱领域 · 头顶咒环", "衰弱领域_节点02_0.65秒.png"),
        ("冰锥术 · 附身寒霜", "冰锥术_节点03_0.65秒.png"),
    ]
    canvas = Image.new("RGB", (1260, 1120), (17, 25, 36))
    draw = ImageDraw.Draw(canvas)
    title = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 32)
    label = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 21)
    small = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 18)
    draw.text((28, 18), "LxARPG  当前技能流程 · 真实单元渲染", font=title, fill=(228, 237, 245))
    draw.text((28, 62), "独立模型 + 分层粒子 · 球体 / 外焰 / 拖尾 / 火星 · UE 5.8 GPU 实际截图", font=small, fill=(158, 184, 205))
    for index, (name, filename) in enumerate(entries):
        col, row = index % 3, index // 3
        x, y = 24 + col * 412, 103 + row * 332
        draw.rounded_rectangle((x, y, x + 396, y + 318), radius=10, fill=(28, 41, 55), outline=(70, 92, 110))
        source = Image.open(render / filename).convert("RGB")
        # 仅去掉空白边缘以便观察细节，不修改颜色、发光或模型形状。
        mask = source.convert("L").point(lambda value: 255 if value > 24 else 0)
        bounds = mask.getbbox()
        if bounds:
            left, top, right, bottom = bounds
            source = source.crop((max(0, left - 24), max(0, top - 24), min(source.width, right + 24), min(source.height, bottom + 24)))
        source.thumbnail((376, 262), Image.Resampling.LANCZOS)
        tile = Image.new("RGB", (384, 274), (0, 0, 0))
        tile.paste(source, ((384 - source.width) // 2, (274 - source.height) // 2))
        canvas.paste(tile, (x + 6, y + 6))
        draw.text((x + 12, y + 286), name, font=label, fill=(218, 233, 243))
    destination = output / "重做技能预览.png"
    canvas.save(destination)
    print(destination)


if __name__ == "__main__":
    make_runtime_sheet()
