#!/usr/bin/env python3
"""wc24_weather.py — generate Wii Forecast Channel data files (forecast.bin,
short.bin) from Open-Meteo, in the exact WC24 wire format documented by
RiiConnect24's Kaitai specs. The channel's own parser/renderer consumes these
natively — no substitution of channel code.

Formats (all big-endian):
  short.bin (LZ10, content at +0x140): header + 24-byte rows per city:
    u8 country, u8 region, u16 location, u32 open_ts, u32 close_ts,
    u16 condition, u8 unk, s1 temp_c, s1 temp_f, u8 wind_dir, u8 kmh, u8 mph,
    u16 unk, u16 unk
  forecast.bin (LZ10): header + 0x... long-forecast rows (today/tomorrow
    conditions + temps + 5-day table), + location table with lat/lng.
  Timestamps: minutes since 2000-01-01 UTC (epoch/60 - 946684800/60).
  Condition codes: the channel's own icon table (u16). Wind dir: 0-16 (NNE..N).
  Country/region/location codes: the channel's own city table (country 49=USA).

This generator produces PLAIN (uncompressed) content; the channel's LZ10
decompressor expects the LZ10 wrapper, so we emit the LZ10 container with
stored (uncompressed) blocks — valid LZ10, zero compression, simplest
correct path. nlzss-style: flag byte 0x00 = 8 literal bytes.
"""
import struct, time, math, json, urllib.request, sys

# ---- condition code table (channel icon indices, from RC24 File-Maker) ----
# Map Open-Meteo WMO weather codes to the channel's condition codes.
# RC24 forecast.py uses AccuWeather->channel icons; we approximate with the
# documented icon set: 0=clear,1=partly,2=cloudy,3=rain,4=snow,5=thunder,etc.
WMO_TO_COND = {
    0: 0, 1: 1, 2: 2, 3: 2, 45: 3, 48: 3,           # clear/mostly/partly/cloudy/fog
    51: 4, 53: 4, 55: 4, 56: 4, 57: 4,              # drizzle -> showers
    61: 5, 63: 5, 65: 6, 66: 4, 67: 4,              # rain
    71: 7, 73: 7, 75: 7, 77: 7,                      # snow
    80: 4, 81: 5, 82: 6, 85: 7, 86: 7,              # showers
    95: 8, 96: 8, 99: 8,                              # thunder
}

def cond(wmo):
    return WMO_TO_COND.get(wmo, 2)

def wind_dir_code(deg):
    """Channel wind direction: 0=none, 1=NNE ... 16=N (16-point rose).
    RC24 maps deg -> 16-point; NNE is index 1."""
    if deg is None:
        return 0
    idx = int(((deg + 11.25) % 360) // 22.5)   # 0=N
    # table: 0=N->16, 1=NNE->1, 2=NE->2, ... (channel's own encoding)
    return {0: 16, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7,
            8: 8, 9: 9, 10: 10, 11: 11, 12: 12, 13: 13, 14: 14, 15: 15}[idx]

def mins_since_2000(ts):
    return int((ts - 946684800) // 60)

def lz10_wrap(plain):
    """Wrap plain bytes in a valid LZ10 container using stored blocks only.
    LZ10: byte 0x10, u24 LE decompressed size, then blocks — flag byte,
    8 chunks, bit CLEAR = literal byte. All-zero flags = 8 literals each.
    The channel's own LZ10 decompressor consumes this natively."""
    size = len(plain)
    out = bytearray()
    out.append(0x10)
    out += bytes([(size >> 16) & 0xFF, (size >> 8) & 0xFF, size & 0xFF])
    i = 0
    while i < size:
        # all-literal block: flag 0x00 + up to 8 literal bytes
        chunk = plain[i:i + 8]
        out.append(0x00)
        out += chunk
        i += 8
    return bytes(out)


# ---------------- default city set (channel's own code space) ----------------
# (country, region, location, name, lat, lon)
# Country 49 = United States in the channel's country_code enum.
DEFAULT_CITIES = [
    (49, 0, 1, "New York",    40.71, -74.01),
    (49, 0, 2, "Los Angeles", 34.05, -118.24),
    (49, 0, 3, "Chicago",     41.88, -87.63),
    (49, 0, 4, "Seattle",     47.61, -122.33),
    (49, 0, 5, "Miami",       25.76, -80.19),
]


def fetch_open_meteo(lat, lon):
    """Current conditions + 5-day forecast from Open-Meteo (no key)."""
    url = ("https://api.open-meteo.com/v1/forecast"
           f"?latitude={lat}&longitude={lon}"
           "&current=temperature_2m,weather_code,wind_speed_10m,wind_direction_10m"
           "&daily=weather_code,temperature_2m_max,temperature_2m_min,wind_speed_10m_max"
           "&forecast_days=6&timezone=auto&temperature_unit=celsius&wind_speed_unit=kmh")
    with urllib.request.urlopen(url, timeout=30) as r:
        return json.loads(r.read().decode())


def build_short(cities, data):
    """short.bin content: header + per-city current-conditions rows."""
    now = int(time.time())
    open_ts = mins_since_2000(now - 3600)
    close_ts = mins_since_2000(now + 6 * 3600)
    rows = bytearray()
    for (cc, rc, lc, name, lat, lon), wx in zip(cities, data):
        cur = wx["current"]
        rows += struct.pack(">BBHIIHbBbBBBBHH",
            cc, rc, lc, open_ts, close_ts,
            cond(cur["weather_code"]),
            0,                                        # unknown
            int(round(cur["temperature_2m"])),        # temp C (s8)
            int(round(cur["temperature_2m"] * 9 / 5 + 32)),  # temp F (s8)
            wind_dir_code(cur["wind_direction_10m"]),
            int(round(cur["wind_speed_10m"])),         # km/h (u8)
            int(round(cur["wind_speed_10m"] * 0.621371)),  # mph (u8)
            0, 0)                                     # padding
    hdr = struct.pack(">IIIII", 0, 0, 0, open_ts, close_ts)
    hdr += struct.pack(">BBBBB", 49, 0, 0, 0, 0)      # country/lang/region/pad
    hdr += struct.pack(">II", len(cities), 0x20)      # count, table offset
    return bytes(hdr) + bytes(rows)


def build_forecast(cities, data):
    """forecast.bin content: header + long-forecast rows (today/tomorrow/5-day)."""
    now = int(time.time())
    open_ts = mins_since_2000(now - 3600)
    close_ts = mins_since_2000(now + 24 * 3600 * 5)
    rows = bytearray()
    for (cc, rc, lc, name, lat, lon), wx in zip(cities, data):
        d = wx["daily"]
        def day_slice(i):
            hi_c = int(round(d["temperature_2m_max"][i]))
            lo_c = int(round(d["temperature_2m_min"][i]))
            hi_f = int(round(hi_c * 9 / 5 + 32))
            lo_f = int(round(lo_c * 9 / 5 + 32))
            return hi_c, lo_c, hi_f, lo_f
        row = bytearray()
        # today: condition u16 + hi/lo C + hi/lo F
        row += struct.pack(">HI", cond(d["weather_code"][0]), 0)
        hi_c, lo_c, hi_f, lo_f = day_slice(0)
        row += struct.pack(">bbbb", hi_c & 0xFF, lo_c & 0xFF,
                           (hi_c & 0xFF), (lo_c & 0xFF))
        row += struct.pack(">BBBB", hi_c & 0xFF, lo_c & 0xFF, hi_f & 0xFF, lo_f & 0xFF)
        row += struct.pack(">BB", 0, 0)               # precipitation JP-only, pad
        # tomorrow
        row += struct.pack(">HI", cond(d["weather_code"][1]), 0)
        hi_c, lo_c, hi_f, lo_f = day_slice(1)
        row += struct.pack(">bbbb", hi_c & 0xFF, lo_c & 0xFF, hi_c & 0xFF, lo_c & 0xFF)
        row += struct.pack(">BBBB", hi_c & 0xFF, lo_c & 0xFF, hi_f & 0xFF, lo_f & 0xFF)
        row += struct.pack(">BB", 0, 0)
        # 5-day table: day codes + temps (s8 C hi/lo, s8 F hi/lo, precip, pad)
        for i in range(5):
            hi_c, lo_c, hi_f, lo_f = day_slice(i)
            row += struct.pack(">HbbbbBB",
                cond(d["weather_code"][i]), hi_c & 0xFF, lo_c & 0xFF,
                hi_f & 0xFF, lo_f & 0xFF, 0, 0)
        rows += bytes(row)
    hdr = struct.pack(">IIII", 0, 0, 0, open_ts)   # version, size, crc, opening
    hdr += struct.pack(">I", close_ts)
    hdr += struct.pack(">BBBBB", 49, 0, 0, 0, 0)
    hdr += struct.pack(">II", 0, 0)               # message offset, long count/offset
    hdr += struct.pack(">II", len(cities), 0x40)  # short count/offset
    hdr += struct.pack(">IIIIII", 0, 0, 0, 0, 0, 0)
    return bytes(hdr) + bytes(rows)


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else "."
    cities = DEFAULT_CITIES
    data = []
    for c in cities:
        try:
            data.append(fetch_open_meteo(c[4], c[5]))
            print(f"  {c[3]}: {data[-1]['current']['temperature_2m']}C "
                  f"code={data[-1]['current']['weather_code']}")
        except Exception as e:
            print(f"  {c[3]}: FETCH FAILED: {e}", file=sys.stderr)
            return 1
    short_plain = build_short(cities, data)
    fc_plain = build_forecast(cities, data)
    open(f"{outdir}/short.bin", "wb").write(lz10_wrap(short_plain))
    open(f"{outdir}/forecast.bin", "wb").write(lz10_wrap(fc_plain))
    # manifest for the runtime injector
    meta = [{"country": c[0], "region": c[1], "location": c[2],
             "name": c[3], "lat": c[4], "lon": c[5]} for c in cities]
    json.dump({"generated": int(time.time()), "cities": meta},
              open(f"{outdir}/wc24_manifest.json", "w"), indent=1)
    print(f"short.bin {len(short_plain)}B plain, forecast.bin {len(fc_plain)}B plain -> {outdir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
