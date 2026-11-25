from telemetry_monitor.protocol import (
    FRAME_ENV,
    FRAME_LOG,
    Decoder,
    crc16_ccitt,
    decode_frame,
    encode_frame,
)


def test_crc_vector():
    assert crc16_ccitt(b"123456789") == 0x29B1


def test_roundtrip_and_crc_error():
    raw = encode_frame(FRAME_LOG, 7, b"\x01\x02PONG")
    frame = decode_frame(raw)
    assert frame is not None
    assert frame.seq == 7
    assert frame.kind == "log"
    assert frame.fields["text"] == "PONG"

    bad = encode_frame(FRAME_LOG, 7, b"\x01\x02PONG", corrupt=True)
    assert decode_frame(bad) is None


def test_decoder_resyncs_and_logs_crc():
    payload = b"\x00" * 14
    good = encode_frame(FRAME_ENV, 1, payload)
    bad = encode_frame(FRAME_ENV, 2, payload, corrupt=True)
    blob = b"\x00\x11noise" + bad + good
    decoder = Decoder()
    found = decoder.feed(blob)
    assert decoder.errors
    assert any("crc" in err for err in decoder.errors)
    assert len(found) == 1
    assert found[0].seq == 1
    assert found[0].kind == "env"


def test_partial_feed():
    raw = encode_frame(FRAME_LOG, 3, b"\x01\x01hi")
    decoder = Decoder()
    assert decoder.feed(raw[:4]) == []
    found = decoder.feed(raw[4:])
    assert len(found) == 1
    assert found[0].fields["text"] == "hi"
