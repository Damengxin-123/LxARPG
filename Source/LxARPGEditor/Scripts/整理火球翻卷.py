"""将真实火球连续帧整理为动态预览和改形前后对照，不修改引擎渲染内容。"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


def visible_bounds(source):
    """只识别有效火焰像素，用于裁去黑色空白边缘。"""
    return source.convert("L").point(lambda value: 255 if value > 24 else 0).getbbox()


def fit_tile(source, width, height):
    """等比摆放引擎截图，保留火球和拖尾的真实比例。"""
    source = source.copy()
    bounds = visible_bounds(source)
    if bounds:
        left, top, right, bottom = bounds
        source = source.crop((max(0, left - 22), max(0, top - 22), min(source.width, right + 22), min(source.height, bottom + 22)))
    scale = min(width / source.width, height / source.height)
    source = source.resize((round(source.width * scale), round(source.height * scale)), Image.Resampling.LANCZOS)
    tile = Image.new("RGB", (width, height), (0, 0, 0))
    tile.paste(source, ((width - source.width) // 2, (height - source.height) // 2))
    return tile


def make_fireball_preview():
    """用0.05秒间隔的实际模拟帧展示连续卷焰，同时生成静态前后对照。"""
    project = Path(__file__).resolve().parents[3]
    output = project / "Saved" / "技能美术"
    render = output / "火球翻卷预览"
    frames = []
    sources = []
    for frame in range(5, 101, 5):
        source = Image.open(render / f"火球术_节点02_{frame / 100:.2f}秒.png").convert("RGB")
        sources.append(source)
    bounds = [visible_bounds(source) for source in sources]
    bounds = [bound for bound in bounds if bound]
    crop = (max(0, min(bound[0] for bound in bounds) - 24), max(0, min(bound[1] for bound in bounds) - 24),
            min(sources[0].width, max(bound[2] for bound in bounds) + 24), min(sources[0].height, max(bound[3] for bound in bounds) + 24))
    title_font = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 27)
    label_font = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 19)
    for source in sources:
        source = source.crop(crop)
        scale = min(820 / source.width, 356 / source.height)
        source = source.resize((round(source.width * scale), round(source.height * scale)), Image.Resampling.LANCZOS)
        canvas = Image.new("RGB", (860, 430), (17, 25, 36))
        draw = ImageDraw.Draw(canvas)
        draw.text((22, 13), "火球 · 包裹球体 / 收尖翻卷 / 飘散余焰", font=title_font, fill=(229, 238, 246))
        canvas.paste(source, ((860 - source.width) // 2, 61 + (356 - source.height) // 2))
        frames.append(canvas)
    frames[0].save(output / "火球翻卷.gif", save_all=True, append_images=frames[1:], duration=50, loop=0, disposal=2)
    comparison = Image.new("RGB", (1280, 492), (17, 25, 36))
    draw = ImageDraw.Draw(comparison)
    draw.text((24, 16), "火球拖尾改形 · UE 5.8 真实技能蓝图渲染", font=title_font, fill=(229, 238, 246))
    before = Image.open(output / "拖尾改形前备份" / "原拖尾.png").convert("RGB")
    after = Image.open(render / "火球术_节点02_0.65秒.png").convert("RGB")
    for index, (source, label) in enumerate([(before, "修改前 · 平行等宽条块"), (after, "修改后 · 立体包焰与收尖翻卷")]):
        x = 20 + index * 636
        comparison.paste(fit_tile(source, 612, 354), (x, 73))
        draw.text((x + 12, 442), label, font=label_font, fill=(210, 227, 242))
    comparison.save(output / "火球拖尾对照.png")
    print(output / "火球翻卷.gif")
    print(output / "火球拖尾对照.png")


if __name__ == "__main__":
    make_fireball_preview()
