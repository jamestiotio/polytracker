import math
from typing import Callable

from PIL import Image


def file_diff(
    num_bytes: int, in_first: Callable[[int], bool], in_second: Callable[[int], bool], aspect_ratio: float = 1.61803398875
) -> Image:
    height = max(int(math.ceil(math.sqrt(aspect_ratio) * math.sqrt(num_bytes))), 1)
    width = max(int(math.ceil(num_bytes / height)), 1)
    while width * height < num_bytes:
        height += 1
    image = Image.new(size=(width, height), mode="RGB", color=(255, 255, 255))
    for i in range(num_bytes):
        first = in_first(i)
        second = in_second(i)
        if not (first and second):
            row = i // width
            col = i % width
            if first:
                image.putpixel((col, row), (0, 0, 255))
            elif second:
                image.putpixel((col, row), (255, 0, 0))
            else:
                image.putpixel((col, row), (0, 0, 0))
    return image
