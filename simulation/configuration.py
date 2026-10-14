"""Strict JSON startup boundary; production firmware validates the encoded profile."""
import json
import re
import struct
from pathlib import Path

MAX_JSON_BYTES = 65536
USAGES = dict(zip(('X', 'Y', 'Z', 'Rx', 'Ry', 'Rz', 'Slider', 'Dial', 'Wheel'), range(48, 57)))


class ConfigurationError(ValueError):
    pass


def fail(path, message):
    raise ConfigurationError(f'{path}: {message}')


def obj(value, path, allowed, required=()):
    if not isinstance(value, dict):
        fail(path, 'expected an object')
    for key in value:
        if key not in allowed:
            fail(f'{path}.{key}', 'unknown field')
    for key in required:
        if key not in value:
            fail(f'{path}.{key}', 'required field missing')
    return value


def integer(value, path, low, high):
    if type(value) is not int or not low <= value <= high:
        fail(path, f'expected an integer in {low}..{high}')
    return value


def boolean(value, path):
    if type(value) is not bool:
        fail(path, 'expected true or false')
    return value


def array(value, path, maximum):
    if not isinstance(value, list) or len(value) > maximum:
        fail(path, f'expected an array with at most {maximum} entries')
    return value


def indexes(value, path, maximum, upper):
    result = [integer(x, f'{path}[{i}]', 0, upper) for i, x in enumerate(array(value, path, maximum))]
    if len(set(result)) != len(result):
        fail(path, 'duplicate entries')
    return result


def unique_keys(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            fail(key, 'duplicate JSON field')
        result[key] = value
    return result


def read_configuration(path):
    with Path(path).open('rb') as stream:
        data = stream.read(MAX_JSON_BYTES + 1)
    if len(data) > MAX_JSON_BYTES:
        fail(str(path), f'JSON exceeds {MAX_JSON_BYTES} bytes')
    try:
        value = json.loads(data.decode('utf-8'), object_pairs_hook=unique_keys,
                           parse_constant=lambda x: fail(str(path), f'invalid JSON constant {x}'))
    except (UnicodeError, json.JSONDecodeError, RecursionError) as error:
        fail(str(path), f'invalid UTF-8 JSON: {error}')
    return encode_configuration(value)


def encode_configuration(value):
    obj(value, 'device', {'identity', 'name', 'inputs', 'expansion_banks', 'initial_samples', 'outputs'},
        {'identity', 'name', 'inputs'})
    identity = value['identity']
    if not isinstance(identity, str) or not re.fullmatch('[0-9a-f]{32}', identity) or identity in ('0' * 32, 'f' * 32):
        fail('identity', 'expected a nonzero, non-ff 32-character lowercase hexadecimal identity')
    name = value['name']
    if not isinstance(name, str) or not name or any(ord(c) < 32 or ord(c) == 127 for c in name):
        fail('name', 'expected a nonempty name without control characters')
    try:
        encoded_name = name.encode('utf-8')
    except UnicodeError:
        fail('name', 'invalid Unicode')
    if len(encoded_name) > 63:
        fail('name', 'must occupy at most 63 UTF-8 bytes')
    banks = indexes(value.get('expansion_banks', []), 'expansion_banks', 8, 7)
    mask = sum(1 << i for i in banks)
    inputs = obj(value['inputs'], 'inputs', {'buttons', 'axes', 'hats', 'encoders', 'selectors'})
    limits = {'buttons': 128, 'axes': 8, 'hats': 4, 'encoders': 8, 'selectors': 8}
    controls = {key: array(inputs.get(key, []), f'inputs.{key}', limit) for key, limit in limits.items()}
    count = {key: len(items) for key, items in controls.items()}
    effective_buttons = count['buttons'] + 2 * count['encoders'] + 3 * count['selectors']
    if effective_buttons > 128:
        fail('inputs', 'buttons plus encoder/selector outputs exceed 128 HID buttons')
    claimed = {}

    def contact(number, path, upper=151):
        number = integer(number, path, 0, upper)
        if number >= 24 and (number - 24) // 16 not in banks:
            fail(path, f'input {number} requires expansion bank {(number - 24) // 16}')
        if number in claimed:
            fail(path, f'input {number} already used by {claimed[number]}')
        claimed[number] = path
        return number

    record = bytearray(b'OSGC' + struct.pack('<I', 1) + encoded_name.ljust(64, b'\0'))
    record += bytes([count[key] for key in limits] + [mask])
    for i, button in enumerate(controls['buttons']):
        path = f'inputs.buttons[{i}]'
        obj(button, path, {'input', 'mode', 'exclusive_group'}, {'input'})
        mode = button.get('mode', 'momentary')
        if mode not in ('momentary', 'maintained'):
            fail(path + '.mode', 'expected momentary or maintained')
        group = integer(button.get('exclusive_group', 0), path + '.exclusive_group', 0, 255)
        if mode == 'maintained' and group:
            fail(path, 'maintained buttons cannot belong to an exclusive group')
        record += bytes([contact(button['input'], path + '.input'), mode == 'maintained', group])
    record += bytes((128 - count['buttons']) * 3)
    channels, usages = set(), set()
    for i, axis in enumerate(controls['axes']):
        path = f'inputs.axes[{i}]'
        obj(axis, path, {'channel', 'usage', 'min', 'max', 'neutral', 'inverted', 'deadzone'}, {'channel', 'usage'})
        channel = integer(axis['channel'], path + '.channel', 0, 7)
        usage = USAGES.get(axis['usage']) if isinstance(axis['usage'], str) else axis['usage']
        usage = integer(usage, path + '.usage', 48, 56)
        if channel in channels or usage in usages:
            fail(path, 'axis channels and HID usages must be unique')
        channels.add(channel); usages.add(usage)
        low = integer(axis.get('min', 0), path + '.min', 0, 4094)
        high = integer(axis.get('max', 4095), path + '.max', low + 1, 4095)
        neutral = integer(axis.get('neutral', 0), path + '.neutral', 0, 65535)
        inverted = boolean(axis.get('inverted', False), path + '.inverted')
        deadzone = integer(axis.get('deadzone', 0), path + '.deadzone', 0, 32767)
        record += struct.pack('<BBHHHBH', channel, usage, low, high, neutral, inverted, deadzone)
    record += bytes((8 - count['axes']) * 11)
    for i, hat in enumerate(controls['hats']):
        path = f'inputs.hats[{i}]'
        obj(hat, path, {'up', 'right', 'down', 'left'}, {'up', 'right', 'down', 'left'})
        record += bytes(contact(hat[key], path + '.' + key) for key in ('up', 'right', 'down', 'left'))
    record += bytes((4 - count['hats']) * 4)
    for kind, fields, upper in [('encoders', ('a', 'b'), 23), ('selectors', ('low', 'high'), 151)]:
        for i, item in enumerate(controls[kind]):
            path = f'inputs.{kind}[{i}]'
            obj(item, path, fields, fields)
            record += bytes(contact(item[key], path + '.' + key, upper) for key in fields)
        record += bytes((8 - count[kind]) * 2)
    assert len(record) == 598
    samples = obj(value.get('initial_samples', {}), 'initial_samples', {'axes', 'closed_contacts', 'expansion_available'})
    axes = array(samples.get('axes', []), 'initial_samples.axes', 8)
    axes = [integer(raw, f'initial_samples.axes[{i}]', 0, 4095) for i, raw in enumerate(axes)]
    axes += [0] * (8 - len(axes))
    closed = indexes(samples.get('closed_contacts', []), 'initial_samples.closed_contacts', 152, 151)
    digital, expanded = 0, [0] * 8
    for number in closed:
        if number < 24:
            digital |= 1 << number
        else:
            bank, pin = divmod(number - 24, 16)
            if bank not in banks:
                fail('initial_samples.closed_contacts', f'input {number} uses disabled expansion bank {bank}')
            expanded[bank] |= 1 << pin
    available = indexes(samples.get('expansion_available', banks), 'initial_samples.expansion_available', 8, 7)
    if not set(available).issubset(banks):
        fail('initial_samples.expansion_available', 'available banks must be configured in expansion_banks')
    outputs = value.get('outputs', 'simulated-leds')
    displays = []
    if isinstance(outputs, dict):
        obj(outputs, 'outputs', {'leds', 'displays'})
        displays = array(outputs.get('displays', []), 'outputs.displays', 4)
        leds = outputs.get('leds', 'board-status')
    else:
        leds = outputs
    if leds not in ('simulated-leds', 'board-status'):
        fail('outputs.leds', 'expected simulated-leds or board-status')
    ports = bytearray()
    # Firmware reserves every builtin LED name across board variants.
    action_ids = {'led.rgb.set_color', 'led.boolean.set_enabled', 'led.status.set_mode'}
    endpoints = set()
    for i, display in enumerate(displays):
        path = f'outputs.displays[{i}]'
        obj(display, path, {'id', 'label', 'endpoint', 'columns', 'rows', 'encoding', 'max_bytes'},
            {'id', 'label', 'endpoint', 'columns'})
        identifiers = []
        for key in ('id', 'endpoint'):
            text = display[key]
            if not isinstance(text, str) or not re.fullmatch(r'[A-Za-z0-9._-]{1,31}', text):
                fail(path + '.' + key, 'expected a 1..31 byte ASCII identifier')
            identifiers.append(text)
        if identifiers[0] in action_ids or identifiers[1] in endpoints:
            fail(path, 'duplicate action ID or display endpoint')
        action_ids.add(identifiers[0]); endpoints.add(identifiers[1])
        label = display['label']
        if not isinstance(label, str) or not label or any(ord(c) < 32 or 127 <= ord(c) <= 159 for c in label):
            fail(path + '.label', 'expected a nonempty UTF-8 label without controls')
        try:
            encoded = label.encode('utf-8')
        except UnicodeError:
            fail(path + '.label', 'invalid Unicode')
        if len(encoded) > 63:
            fail(path + '.label', 'must occupy at most 63 UTF-8 bytes')
        columns = integer(display['columns'], path + '.columns', 1, 63)
        rows = integer(display.get('rows', 1), path + '.rows', 1, 4)
        cells = columns * rows
        if cells > 63:
            fail(path, 'display surface must contain at most 63 cells')
        encoding = display.get('encoding', 'ascii')
        if encoding not in ('ascii', 'utf8'):
            fail(path + '.encoding', 'expected ascii or utf8')
        max_bytes = integer(display.get('max_bytes', cells if encoding == 'ascii' else 63),
                            path + '.max_bytes', cells, 63)
        ports += identifiers[0].encode().ljust(32, b'\0') + encoded.ljust(64, b'\0')
        ports += identifiers[1].encode().ljust(32, b'\0') + bytes([columns, rows, encoding == 'ascii', max_bytes])
    ports += bytes((4 - len(displays)) * 132)
    payload = b'OSGSIM03' + bytes.fromhex(identity) + record
    payload += struct.pack('<I8HB8H', digital, *axes, sum(1 << i for i in available), *expanded)
    payload += bytes([leds == 'board-status', len(displays)]) + ports
    return identity, payload
