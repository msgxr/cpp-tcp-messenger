#!/usr/bin/env python3
"""Gercek C++ uygulamalarini ve bagimsiz TCP paketlerini dogrular; Python 3.8+."""
import contextlib
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import signal
import socket
import struct
import subprocess
import tempfile
import threading
import time
import uuid
import zlib

TEST_DIR = Path(__file__).resolve().parent
BASE = TEST_DIR.parent
ROOT = BASE.parent.parent
BUILD = BASE / "02_Derleme_ve_Calistirma_Dosyalari"
SRC = BASE / "01_Kaynak_Kod_Dosyalari"
BIN = BUILD / "Derlenmis_Uygulama_Dosyalari"
DATA = TEST_DIR / "Test_Verileri"
OUTPUT = Path("702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/04_Uygulama_Ciktilari/Alinan_Dosyalar")
HEADER = struct.Struct("!BHII")
MAX_TEXT = 1024 * 1024
MAX_IMAGE = 50 * 1024 * 1024
ACTIVE = []
RESULTS = []
LOGS = []
ANSI = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")
WORK = None


def until(predicate, seconds=12, description="beklenen sonuc"):
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        if predicate():
            return
        time.sleep(0.025)
    raise AssertionError("Zaman asimi: " + description)


class App:
    def __init__(self, role):
        self.role = role
        self.data = bytearray()
        self.process = subprocess.Popen(
            [str(BIN / role)], cwd=str(WORK), stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            env=dict(os.environ, LC_ALL="C.UTF-8", LANG="C.UTF-8", COLORTERM="truecolor"),
            bufsize=0, start_new_session=True)
        self.reader = threading.Thread(target=self._read, daemon=True)
        self.reader.start()
        ACTIVE.append(self)

    def _read(self):
        while True:
            block = self.process.stdout.read(4096)
            if not block:
                return
            self.data.extend(block)

    @property
    def text(self):
        return ANSI.sub("", bytes(self.data).decode("utf-8", "replace"))

    def wait(self, text, count=1, seconds=12):
        until(lambda: self.text.count(text) >= count, seconds, self.role + ": " + text)

    def send(self, text):
        data = memoryview((text + "\n").encode("utf-8"))
        while data:
            n = os.write(self.process.stdin.fileno(), data)
            data = data[n:]

    def exited(self, code=0):
        actual = self.process.wait(timeout=8)
        self.reader.join(timeout=2)
        assert actual == code, "Cikis kodu: %s, beklenen: %s\n%s" % (actual, code, self.text[-2000:])

    def stop(self):
        if self.process.poll() is None:
            os.killpg(self.process.pid, signal.SIGTERM)
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                os.killpg(self.process.pid, signal.SIGKILL)
                self.process.wait(timeout=2)
        self.reader.join(timeout=2)
        LOGS.append("\n--- " + self.role + " ---\n" + self.text)
        self.process.stdin.close()
        self.process.stdout.close()


def packet(kind, payload=b"", name=b"", crc=None, size=None):
    if isinstance(payload, str):
        payload = payload.encode("utf-8")
    checksum = zlib.crc32(payload) & 0xffffffff if crc is None else crc
    return HEADER.pack(kind, len(name), len(payload) if size is None else size, checksum) + name + payload


@contextlib.contextmanager
def receiver():
    app = App("server")
    app.wait("İstemci bekleniyor")
    peer = socket.create_connection(("127.0.0.1", 5000), timeout=30)
    peer.settimeout(30)
    app.wait("İstemci bağlandı")
    try:
        yield app, peer
    finally:
        peer.close()


def pair():
    server = App("server")
    server.wait("İstemci bekleniyor")
    client = App("client")
    client.wait("Sunucuya bağlandı")
    server.wait("İstemci bağlandı")
    return server, client


def received():
    folder = WORK / OUTPUT
    return sorted(folder.glob("*")) if folder.exists() else []


def png():
    return (DATA / "piksel_test.png").read_bytes()


def crc_ok(app, count=1):
    app.wait("Metin bütünlüğü CRC32 ile doğrulandı.", count)


def wire_rejection(header, expected):
    with receiver() as (app, peer):
        peer.sendall(header)
        app.wait(expected)
        app.exited()


def protocol_unit():
    executable = WORK / "protocol_test"
    subprocess.run(["g++", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-std=c++17", "-pthread",
                    "-I" + str(SRC), str(TEST_DIR / "protocol_test.cpp"), str(SRC / "protocol.cpp"),
                    "-o", str(executable)], check=True, capture_output=True, timeout=60)
    subprocess.run([str(executable)], check=True, capture_output=True, timeout=10)


def preview(ext):
    # Chafa 1.2, yardim metnini basarili yazsa da 1 dondurebilir.
    help_result = subprocess.run(["chafa", "--help"], stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, check=False, timeout=10)
    help_text = help_result.stdout
    format_args = ["--format", "symbols"] if b"--format" in help_text else []
    result = subprocess.run(["chafa"] + format_args + ["--colors", "full", "--symbols", "block",
                             "--size", "40x16", "--", str(DATA / ("piksel_test." + ext))],
                            check=True, capture_output=True, timeout=15)
    assert b"\x1b[" in result.stdout and len(result.stdout) > 100, "ANSI piksel onizlemesi uretilmedi"


def texts():
    server, client = pair()
    for i, message in enumerate(["SELAM", "G", "Türkçe: şğüİöç"]):
        client.send(message)
        server.wait(message)
        crc_ok(server, i + 1)
    server.send("SUNUCU YANITI")
    client.wait("SUNUCU YANITI")
    crc_ok(client)


def long_text():
    server, client = pair()
    text = "Türkçe şğüİöç " * 30 + "MESAJIN_SONU"
    client.send(text)
    server.wait("MESAJIN_SONU")
    crc_ok(server)
    assert "..." not in server.text.split("GELEN MESAJ")[-1]


def commands():
    server, client = pair()
    for app in (server, client):
        app.send("!yardim")
        app.wait("KOMUTLAR")
        app.send("!durum")
        app.wait("TCP bağlantısı açık.")
        app.send("!temizle")
        app.wait("TCP METİN", 2)
        app.send("!resim")
        app.wait("Kullanım: !resim")
    client.send("KOMUTLARDAN_SONRA")
    server.wait("KOMUTLARDAN_SONRA")


def pictures(ext):
    server, client = pair()
    source = DATA / ("piksel_test." + ext)
    for sender, target in ((client, server), (server, client)):
        sender.send("!resim " + str(source))
        target.wait("DOĞRULANDI", 1)
        target.wait("Sen >", 2)
    until(lambda: len(received()) == 2)
    assert all(p.read_bytes() == source.read_bytes() for p in received())
    for app in (server, client):
        until(lambda app=app: b"\x1b[38;2;" in bytes(app.data) or b"\x1b[48;2;" in bytes(app.data))
        assert "önizlemesi gösterilemedi" not in app.text


def duplicates():
    server, client = pair()
    source = str(DATA / "piksel_test.png")
    # Iki uygulama ayni dizine ayni isimle ayni anda kaydetmeye calisir.
    a = threading.Thread(target=server.send, args=("!resim " + source,))
    b = threading.Thread(target=client.send, args=("!resim " + source,))
    a.start(); b.start(); a.join(); b.join()
    server.wait("DOĞRULANDI"); client.wait("DOĞRULANDI")
    assert len(received()) == 2
    assert len({p.name for p in received()}) == 2
    assert all(p.read_bytes() == png() for p in received())


def quoted_path():
    server, client = pair()
    path = WORK / "Türkçe resim 'deneme'.PNG"
    path.write_bytes(png())
    client.send('!resim "' + str(path) + '"')
    server.wait("DOĞRULANDI")
    assert received()[0].read_bytes() == png()
    until(lambda: b"\x1b[38;2;" in bytes(server.data) or b"\x1b[48;2;" in bytes(server.data))


def invalid_send(which):
    server, client = pair()
    if which == "missing":
        path = WORK / "olmayan.png"
        expected = "Dosya bulunamadı"
    elif which == "extension":
        path = WORK / "yanlis.txt"; path.write_bytes(png())
        expected = "Yalnızca JPG/JPEG/PNG"
    elif which == "empty":
        path = WORK / "bos.png"; path.touch()
        expected = "Görsel boş olamaz"
    elif which == "image-size":
        path = WORK / "buyuk.png"
        with path.open("wb") as file: file.truncate(MAX_IMAGE + 1)
        expected = "50 MB'dan büyük olamaz"
    else:
        path = None
        expected = "Metin 1 MB'dan büyük olamaz"
    client.send("!resim " + str(path) if path is not None else "A" * (MAX_TEXT + 1))
    client.wait(expected)
    client.send("HATADAN_SONRA_BAGLANTI_ACIK")
    server.wait("HATADAN_SONRA_BAGLANTI_ACIK")
    crc_ok(server)
    assert not received()


def text_boundary():
    with receiver() as (app, peer):
        app.send("A" * MAX_TEXT)
        raw = bytearray()
        while len(raw) < HEADER.size:
            raw.extend(peer.recv(HEADER.size - len(raw)))
        kind, name_len, size, crc = HEADER.unpack(raw)
        assert (kind, name_len, size) == (1, 0, MAX_TEXT)
        payload = bytearray()
        while len(payload) < size:
            payload.extend(peer.recv(min(65536, size - len(payload))))
        assert payload == b"A" * MAX_TEXT and zlib.crc32(payload) & 0xffffffff == crc


def image_boundary():
    payload = png() + b"\0" * (MAX_IMAGE - len(png()))
    with receiver() as (app, peer):
        peer.sendall(packet(2, payload, b"sinir.png"))
        app.wait("DOĞRULANDI", seconds=35)
        assert len(received()) == 1 and received()[0].stat().st_size == MAX_IMAGE
        assert hashlib.sha256(received()[0].read_bytes()).digest() == hashlib.sha256(payload).digest()


def fragments():
    with receiver() as (app, peer):
        raw = packet(1, "PARÇALI_TCP_şğüİöç")
        for byte in raw:
            peer.sendall(bytes([byte])); time.sleep(0.002)
        app.wait("PARÇALI_TCP_şğüİöç"); crc_ok(app)


def burst():
    with receiver() as (app, peer):
        peer.sendall(b"".join(packet(1, "ARDISIK_%02d" % i) for i in range(30)))
        crc_ok(app, 30)
        for i in range(30): assert "ARDISIK_%02d" % i in app.text


def bad_crc(kind):
    with receiver() as (app, peer):
        if kind == 1:
            peer.sendall(packet(1, "BOZUK_METIN_GOSTERILMEMELI", crc=0))
            app.wait("Metin CRC32 kontrolü başarısız")
            assert "BOZUK_METIN_GOSTERILMEMELI" not in app.text
        else:
            peer.sendall(packet(2, png(), b"bozuk.png", crc=0))
            app.wait("bozuk dosya silindi")
            assert not received()
        peer.sendall(packet(1, "CRC_HATASINDAN_SONRA"))
        app.wait("CRC_HATASINDAN_SONRA"); crc_ok(app)


def incomplete(image):
    with receiver() as (app, peer):
        raw = packet(2, png(), b"yarim.png") if image else packet(1, "YARIM")
        peer.sendall(raw[:HEADER.size + len(b"yarim.png") + 20] if image else raw[:5])
        peer.shutdown(socket.SHUT_WR)
        app.exited()
        assert not received()


def path_safety():
    with receiver() as (app, peer):
        peer.sendall(packet(2, png(), b"../../disari.png"))
        app.wait("DOĞRULANDI")
        assert len(received()) == 1 and received()[0].name == "disari.png"
        assert not (WORK / "disari.png").exists()


def exit_clean():
    server, client = pair()
    client.send("cikis")
    client.exited(); server.exited()


def abrupt_close():
    server, client = pair()
    os.killpg(server.process.pid, signal.SIGKILL)
    server.exited(-signal.SIGKILL)
    client.exited()


def refused():
    client = App("client")
    client.exited(1)
    assert "Bağlantı kurulamadı" in client.text


def crlf():
    server, client = pair()
    client.send("CRLF_SATIRI\r")
    server.wait("CRLF_SATIRI"); crc_ok(server)


def tmux_launcher():
    build = WORK / BUILD.relative_to(ROOT)
    build.mkdir(parents=True)
    for name in ("kur.sh", "calistir.sh", "sunucu.sh", "istemci.sh"):
        shutil.copy2(BUILD / name, build / name)
    shutil.copy2(BUILD / "Makefile", build / "Makefile")
    # Baslatma betiklerini gercek bosluklu/Turkce yolda, yeniden derlemeden sinar.
    (build / "Makefile").write_text("all:\n\t@true\n", encoding="utf-8")
    (build / "Derlenmis_Uygulama_Dosyalari").symlink_to(BIN, target_is_directory=True)
    socket_name = "tcp-check-" + uuid.uuid4().hex
    env = dict(os.environ, TCPMSG_SOCKET=socket_name, TCPMSG_SESSION="test")
    cmd = ["tmux", "-L", socket_name]
    try:
        subprocess.run(["bash", str(build / "calistir.sh"), "--ayrik"], check=True, env=env,
                       capture_output=True, timeout=20)
        panes = subprocess.check_output(cmd + ["list-panes", "-t", "test", "-F", "#{pane_id}"]).decode().splitlines()
        assert len(panes) == 2
        def capture(index):
            return subprocess.check_output(cmd + ["capture-pane", "-p", "-S", "-1000", "-t", panes[index]]).decode("utf-8", "replace")
        until(lambda: "Sunucuya bağlandı" in capture(1), description="tmux istemci baglantisi")
        subprocess.run(cmd + ["send-keys", "-t", panes[1], "-l", "TMUX_YOL_TESTI"], check=True)
        subprocess.run(cmd + ["send-keys", "-t", panes[1], "Enter"], check=True)
        until(lambda: "TMUX_YOL_TESTI" in capture(0), description="tmux metin aktarimi")
        image_command = "!resim " + str(DATA / "piksel_test.png")
        subprocess.run(cmd + ["send-keys", "-t", panes[1], "-l", image_command], check=True)
        subprocess.run(cmd + ["send-keys", "-t", panes[1], "Enter"], check=True)
        until(lambda: "DOĞRULANDI" in capture(0), description="tmux gorsel aktarimi")
        assert len(received()) == 1 and received()[0].read_bytes() == png()
    finally:
        subprocess.run(cmd + ["kill-server"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


TESTS = [
    ("11 bayt baslik, ag bayt sirasi, CRC32 referansi, parcali I/O, EOF ve SIGPIPE", protocol_unit),
    ("PNG renkli piksel onizlemesi", lambda: preview("png")),
    ("JPG renkli piksel onizlemesi", lambda: preview("jpg")),
    ("Iki yonlu metin ve Turkce karakterler", texts),
    ("Uzun Turkce metnin tamamini gosterme", long_text),
    ("Yardim, durum, temizle ve resim komutlari", commands),
    ("Iki yonlu PNG, dosya esligi ve piksel onizlemesi", lambda: pictures("png")),
    ("Iki yonlu JPG, dosya esligi ve piksel onizlemesi", lambda: pictures("jpg")),
    ("Eszamanli ayni isimli dosyalari ayri kaydetme", duplicates),
    ("Bosluklu, Turkce, tirnakli yol ve buyuk harfli PNG uzantisi", quoted_path),
    ("Olmayan dosyayi reddetme ve baglantiyi surdurme", lambda: invalid_send("missing")),
    ("Desteklenmeyen dosyayi reddetme ve baglantiyi surdurme", lambda: invalid_send("extension")),
    ("Bos gorseli reddetme", lambda: invalid_send("empty")),
    ("Tam 1 MiB metin gonderimi ve bagimsiz CRC32 dogrulamasi", text_boundary),
    ("1 MiB ustu metni reddetme ve baglantiyi surdurme", lambda: invalid_send("text-size")),
    ("Tam 50 MiB gorsel alma ve SHA256 esligi", image_boundary),
    ("50 MiB ustu gorseli gondermeden reddetme", lambda: invalid_send("image-size")),
    ("Bayt bayt parcalanmis TCP ust bilgisi ve veri", fragments),
    ("Tek TCP akisi icinde 30 ardisik mesaj", burst),
    ("Bozuk metin CRC32 ve ardindan saglam mesaj", lambda: bad_crc(1)),
    ("Bozuk gorsel CRC32, dosyayi silme ve ardindan saglam mesaj", lambda: bad_crc(2)),
    ("Yarim ust bilgide kilitlenmeden cikis", lambda: incomplete(False)),
    ("Yarim gorselde dosyayi silme ve otomatik cikis", lambda: incomplete(True)),
    ("Gecersiz mesaj turunu reddetme", lambda: wire_rejection(HEADER.pack(9, 0, 0, 0), "Geçersiz mesaj türü")),
    ("Metinde dosya adi alanini reddetme", lambda: wire_rejection(HEADER.pack(1, 1, 0, 0), "Geçersiz metin paketi")),
    ("1 MiB ustu gelen metin basligini reddetme", lambda: wire_rejection(HEADER.pack(1, 0, MAX_TEXT + 1, 0), "Geçersiz metin paketi")),
    ("Bos gelen gorsel adini reddetme", lambda: wire_rejection(HEADER.pack(2, 0, 1, 0), "Geçersiz görsel paketi")),
    ("255 bayt ustu gelen dosya adini reddetme", lambda: wire_rejection(HEADER.pack(2, 256, 1, 0), "Geçersiz görsel paketi")),
    ("50 MiB ustu gelen gorsel basligini reddetme", lambda: wire_rejection(HEADER.pack(2, 5, MAX_IMAGE + 1, 0), "Geçersiz görsel paketi")),
    ("Gelen gorselde desteklenmeyen uzantiyi reddetme", lambda: wire_rejection(packet(2, b"x", b"x.exe"), "uzantısı kabul edilmiyor")),
    ("Dosya adinda ust dizine cikisi engelleme", path_safety),
    ("cikis komutuyla iki uygulamanin otomatik kapanmasi", exit_clean),
    ("Ani sunucu kapanmasinda istemcinin otomatik kapanmasi", abrupt_close),
    ("Sunucu yokken acik baglanti hatasi", refused),
    ("Windows CRLF girdisiyle dogru metin gonderimi", crlf),
    ("Gercek tmux, bosluklu Turkce yol, metin ve gorsel aktarimi", tmux_launcher),
]


def main():
    global WORK
    report_dir = TEST_DIR / "Test_Sonuclari"
    report_dir.mkdir(parents=True, exist_ok=True)
    for tool in ("g++", "chafa", "tmux", "ss", "timeout"):
        if shutil.which(tool) is None:
            raise RuntimeError("Eksik arac: " + tool + "; once kur.sh calistirin.")
    for role in ("server", "client"):
        if not (BIN / role).is_file():
            raise RuntimeError("Once temiz derleme yapin: dogrula.sh")
    with socket.socket() as probe:
        probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            probe.bind(("0.0.0.0", 5000))
        except OSError as error:
            raise RuntimeError("5000/TCP dolu; acik sunucuyu Ctrl+C ile kapatin.") from error
    started = time.monotonic()
    for index, (label, test) in enumerate(TESTS, 1):
        before = time.monotonic()
        ok, detail = True, ""
        with tempfile.TemporaryDirectory(prefix="TCP Test Türkçe ") as temporary:
            WORK = Path(temporary)
            try:
                test()
            except Exception as error:
                ok, detail = False, repr(error)
                if isinstance(error, subprocess.CalledProcessError):
                    output = (error.stdout or b"") + (error.stderr or b"")
                    detail += "\n" + output.decode("utf-8", "replace")[-3000:]
            finally:
                for app in ACTIVE:
                    app.stop()
                ACTIVE.clear()
        RESULTS.append(dict(test=label, passed=ok, seconds=round(time.monotonic() - before, 3), detail=detail))
        print("[%s] %02d/%02d %s%s" % ("OK" if ok else "HATA", index, len(TESTS), label, ": " + detail if detail else ""), flush=True)
    passed = sum(result["passed"] for result in RESULTS)
    summary = dict(date=datetime.datetime.now(datetime.timezone.utc).isoformat(), environment=platform.platform(),
                   passed=passed, total=len(RESULTS), seconds=round(time.monotonic() - started, 3), tests=RESULTS)
    (report_dir / "SON_TEST_RAPORU.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    lines = ["# TCP Uygulaması Test Raporu", "", "Tarih: " + summary["date"], "", "Sonuç: **%d/%d başarılı**" % (passed, len(RESULTS)), "", "| Kontrol | Sonuç |", "|---|---|"]
    lines += ["| %s | %s |" % (r["test"], "BAŞARILI" if r["passed"] else "HATA: " + r["detail"].replace("|", "/")) for r in RESULTS]
    (report_dir / "SON_TEST_RAPORU.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    (report_dir / "uygulama.log").write_text("\n".join(LOGS), encoding="utf-8")
    print("\nSONUC: %d/%d basarili. Rapor: %s" % (passed, len(RESULTS), report_dir), flush=True)
    return 0 if passed == len(RESULTS) else 1


if __name__ == "__main__":
    raise SystemExit(main())
