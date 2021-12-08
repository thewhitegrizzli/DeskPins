"""Batch-mark CHM files with language and version."""

import payload

MAGIC = 0xefda7a00

a = [
    ('DeskPins.chm', 'English v1.30'),
    ('DeskPinsGreek.chm', 'Greek v1.30'),
]

for fn, data in a:
    payload.set(fn, MAGIC, data)
