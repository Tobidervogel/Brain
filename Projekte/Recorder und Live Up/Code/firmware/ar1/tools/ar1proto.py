"""Reference implementation of the AR1 Bluetooth LE session (specification section 3.7).

  python ar1proto.py          rewrite docs/ar1-proto-vectors.json

The firmware (SELFTEST) and the phone app (unit test) check themselves against the same fixed vectors.
"""

import hashlib
import hmac
import json
from pathlib import Path

from cryptography.exceptions import InvalidTag
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

TO_DEVICE, TO_PHONE = 1, 2
OVERHEAD = 4 + 16 + 6          # counter, tag, response header (req, seq, last)
VECTORS = Path(__file__).resolve().parents[3] / "docs" / "ar1-proto-vectors.json"


def session_key(key, nd, np):
    return hmac.new(key, b"AR1-session" + nd + np, hashlib.sha256).digest()


def seal(ks, direction, ctr, plain):
    nonce = bytes([direction]) + bytes(7) + ctr.to_bytes(4, "big")
    return ctr.to_bytes(4, "big") + AESGCM(ks).encrypt(nonce, plain, None)


def unseal(ks, direction, frame, last_ctr):
    """Returns (ctr, plaintext); raises ValueError for a forged, damaged or replayed frame."""
    ctr = int.from_bytes(frame[:4], "big")
    if len(frame) < 20 or ctr <= last_ctr:
        raise ValueError("replayed or malformed frame")
    nonce = bytes([direction]) + bytes(7) + frame[:4]
    try:
        return ctr, AESGCM(ks).decrypt(nonce, frame[4:], None)
    except InvalidTag:
        raise ValueError("wrong key or damaged frame") from None


def response_frames(ks, first_ctr, req, payload, mtu):
    """The JSON answer to command number req, cut into indications that fit the MTU."""
    size = mtu - 3 - OVERHEAD
    parts = [payload[i:i + size] for i in range(0, len(payload), size)] or [b""]
    return [seal(ks, TO_PHONE, first_ctr + seq, req.to_bytes(4, "big") + bytes([seq, seq == len(parts) - 1]) + part)
            for seq, part in enumerate(parts)]


def join_response(ks, frames, last_ctr=0):
    """(req, payload) from the frames of one answer."""
    payload, req = b"", None
    for expected, frame in enumerate(frames):
        last_ctr, plain = unseal(ks, TO_PHONE, frame, last_ctr)
        if plain[4] != expected or (req is not None and plain[:4] != req):
            raise ValueError("answer parts out of order")
        req, payload = plain[:4], payload + plain[6:]
    if not plain[5]:
        raise ValueError("answer incomplete")
    return int.from_bytes(req, "big"), payload


def vectors():
    key = bytes(range(32))
    nd, np = bytes(range(0x40, 0x50)), bytes(range(0x80, 0x90))
    ks = session_key(key, nd, np)
    answer = json.dumps({"ok": True, "state": "recording", "note": "x" * 420}, separators=(",", ":")).encode()
    return {
        "key": key.hex(), "nd": nd.hex(), "np": np.hex(),
        "hello_read": (b"\x01" + nd).hex(), "hello_write": (b"\x01" + np).hex(),
        "ks": ks.hex(),
        "command": {"ctr": 1, "text": "STATUS", "frame": seal(ks, TO_DEVICE, 1, b"STATUS").hex()},
        "command2": {"ctr": 2, "text": "TIMER sleep 600", "frame": seal(ks, TO_DEVICE, 2, b"TIMER sleep 600").hex()},
        "response": {"req": 1, "first_ctr": 1, "mtu": 247, "payload": answer.decode(),
                     "frames": [f.hex() for f in response_frames(ks, 1, 1, answer, 247)]},
    }


if __name__ == "__main__":
    VECTORS.write_text(json.dumps(vectors(), indent=1) + "\n")
    print(f"written: {VECTORS}")
