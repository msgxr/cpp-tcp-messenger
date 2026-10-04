# C++ TCP Metin ve Görüntü Mesajlaşma Uygulaması

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B&logoColor=white)
![Platform](https://img.shields.io/badge/Windows%2011-WSL2%20Ubuntu%2020.04-0078D4?logo=windows)
![Protocol](https://img.shields.io/badge/Protocol-TCP-2ea44f)
![Integrity](https://img.shields.io/badge/Integrity-CRC32-orange)

Windows 11 üzerinde **WSL2 / Ubuntu 20.04** ortamında çalışan, C++17 ile geliştirilmiş TCP tabanlı iki yönlü metin ve JPG/PNG görüntü mesajlaşma uygulamasıdır. Proje; TCP soket programlama, uygulama katmanı çerçeveleme, eksiksiz bayt aktarımı, dosya bütünlüğü denetimi ve eşzamanlı alım/gönderim mantığını uygulamalı olarak göstermeyi amaçlar.

> Çekirdek haberleşme ve dosya aktarım mantığı C++17 ile yazılmıştır. `tmux` ve `chafa` yalnızca terminal sunumu/önizlemesi için yardımcı araçlardır; TCP protokolünün çalışması bunlara bağlı değildir.

## Öne Çıkan Özellikler

- TCP üzerinden çift yönlü metin mesajlaşması
- JPG, JPEG ve PNG görüntü aktarımı
- Gönderici ve alıcı için ayrı C++ iş parçacığı ile eşzamanlı çalışma
- `send()` / `recv()` kısmi aktarım durumlarını yöneten `send_exact()` ve `read_exact()` yardımcıları
- **11 baytlık uygulama üst bilgisi**
- Metin ve görüntülerde **CRC32 bütünlük doğrulaması**
- Bozuk gelen görüntünün otomatik silinmesi
- 1 MB metin ve 50 MB görüntü boyutu sınırı
- Güvenli dosya adı üretimi ve dizin dışına yazmayı engelleme
- Aynı isimli alınan dosyayı ezmeden yeni ad üretme
- ANSI renkli terminal arayüzü
- Gönderim/alım ilerleme çubuğu
- `chafa` ile terminal içinde renkli görüntü önizlemesi
- `tmux` ile tek ekranda SUNUCU | İSTEMCİ görünümü
- Windows PowerShell üzerinden WSL2'ye tek komutla geçiş

## Çalışma Ortamı

| Bileşen | Kullanılan ortam |
|---|---|
| Ana işletim sistemi | Windows 11 |
| Linux çalışma ortamı | WSL2 - Ubuntu 20.04 LTS |
| Dil standardı | C++17 |
| Derleyici | `g++` |
| Haberleşme | TCP / IPv4 |
| Yerel sunucu | `127.0.0.1:5000` |
| Terminal sunumu | ANSI + `tmux` |
| Görüntü önizleme | `chafa` |

## Proje Yapısı

```text
cpp-tcp-messenger/
├── client.cpp                  # TCP istemcisi
├── server.cpp                  # TCP sunucusu
├── protocol.h                  # Mesaj başlığı, CRC32, send/recv yardımcıları
├── protocol.cpp                # Proje kaynak yapısında ayrılmış protokol dosyası
├── chat_common.h               # Metin/görüntü gönderme-alma ortak mantığı
├── ui.h                        # Terminal arayüzü, kartlar, progress ve chafa önizleme
├── Makefile                    # C++17 derleme kuralları
├── calistir.sh                 # WSL/Linux için tek komut başlatıcı
├── calistir.ps1                # Windows PowerShell başlatıcısı
├── sunucu.sh                   # Sunucu başlatıcı
├── istemci.sh                  # İstemci başlatıcı
├── test_resimleri/             # Test görüntüleri
├── alinanlar/                  # Çalışma sırasında otomatik oluşturulur
└── docs/
    ├── PROJE_RAPORU_GUNCEL.txt # Güncel LaTeX rapor kaynağı - TXT kopyası
    └── PROJE_RAPORU_GUNCEL.tex # Aynı raporun doğrudan .tex sürümü
```

## Hızlı Başlangıç - Windows 11 + WSL2

PowerShell'de proje klasörüne geçip:

```powershell
wsl -d Ubuntu-20.04 --cd "$PWD" bash -lc "chmod +x calistir.sh sunucu.sh istemci.sh && ./calistir.sh"
```

İlk çalıştırmada `tmux` veya `chafa` eksikse `calistir.sh` bunları Ubuntu 20.04 içine kurar ve `sudo` parolası isteyebilir.

Alternatif olarak:

```powershell
powershell -ExecutionPolicy Bypass -File .\calistir.ps1
```

## Manuel Derleme ve Çalıştırma

Ubuntu 20.04 terminalinde:

```bash
make clean
make
```

Birinci terminal:

```bash
./server
```

İkinci terminal:

```bash
./client
```

## Kullanım

Normal metin göndermek için doğrudan yazın:

```text
Merhaba TCP
```

Görüntü göndermek için:

```text
!resim test_resimleri/foto1.jpg
```

Komutlar:

| Komut | İşlev |
|---|---|
| `!resim <dosya>` | JPG/JPEG/PNG gönderir |
| `!durum` | Bağlantı durumunu gösterir |
| `!temizle` | Terminal ekranını temizler |
| `!yardim` | Komut yardımını gösterir |
| `cikis` | Bağlantıyı düzenli kapatır |

## Uygulama Protokolü

TCP bir bayt akışıdır; mesaj sınırlarını kendiliğinden bilmez. Bu nedenle uygulama her iletinin başına kendi sabit üst bilgisini ekler.

| Alan | Boyut | Açıklama |
|---|---:|---|
| `type` | 1 bayt | `1`: metin, `2`: görüntü |
| `filename_len` | 2 bayt | Dosya adı uzunluğu; metinde `0` |
| `data_size` | 4 bayt | İçerik boyutu |
| `checksum` | 4 bayt | İçeriğin CRC32 değeri |
| **Toplam sabit başlık** | **11 bayt** | Ağ bayt sırası kullanılır |

Görüntü mesajında 11 baytlık başlığın ardından dosya adı ve dosya baytları gelir. Metin mesajında dosya adı bulunmaz.

```text
+------+--------------+-----------+----------+----------------+------------------+
| type | filename_len | data_size | checksum | filename (N)   | payload (D)      |
| 1 B  | 2 B          | 4 B       | 4 B      | N B            | D B              |
+------+--------------+-----------+----------+----------------+------------------+
```

## Bütünlük ve Güvenlik Kontrolleri

Uygulama internet güvenliği sağlayan bir kriptografik protokol değildir; ancak yerel proje senaryosu için aşağıdaki kontroller uygulanır:

- CRC32 ile aktarım bütünlüğü kontrolü
- Hatalı CRC32 alan görüntünün kaydedilmemesi/silinmesi
- Dosya uzantısının JPG/JPEG/PNG ile sınırlandırılması
- Dosya adının güvenli hale getirilmesi
- `../` gibi dizin kaçışlarının dosya adına taşınmaması
- Aynı isimli dosyanın üzerine yazılmaması
- Metin ve görüntü için üst boyut kontrolleri
- Eksik TCP aktarımında işlemin başarısız kabul edilmesi

> CRC32, kötü niyetli değişikliklere karşı kimlik doğrulama sağlamaz. Şifreleme ve kimlik doğrulama bu ders projesinin kapsamı dışındadır.

## Terminal Görüntü Önizlemesi

Başarıyla alınan bir görüntü `alinanlar/` klasörüne kaydedilir. `chafa` kuruluysa görüntü ayrıca terminal içinde renkli blok/piksel temsiliyle gösterilir.

```text
┌─ GELEN GÖRSEL ────────────────────────────────────┐
  Dosya : alinanlar/foto1.jpg
  Boyut : ... bayt
  CRC32 : 0x........  DOĞRULANDI ✓
└───────────────────────────────────────────────────┘
```

## Doğrulama Senaryoları

- Kısa ve Türkçe karakterli metin aktarımı
- İstemci -> sunucu görüntü aktarımı
- Sunucu -> istemci görüntü aktarımı
- Ardışık metin ve görüntü gönderimi
- JPG ve PNG dosyalarının alınması
- CRC32 bütünlük doğrulaması
- Aynı isimli dosyada çakışmasız yeni ad üretimi
- Geçersiz dosya yolu kontrolü
- Boyut sınırı kontrolü
- Bağlantı kapanışının düzgün yönetilmesi

## Teknik Kapsam

Bu sürüm tek sunucu ve tek istemci için, aynı bilgisayarda yerel TCP haberleşmesini hedefler. Çoklu istemci, kullanıcı hesabı, kalıcı mesaj geçmişi, TLS, uçtan uca şifreleme ve internet üzerinde kimlik doğrulamalı kullanım kapsam dışıdır.

## Derleme Temizliği

```bash
make clean
```

Derlenen `server`, `client` dosyaları ve `alinanlar/` klasörü `.gitignore` ile Git deposuna dahil edilmez.

## Proje Sahibi

**Muhammed Sina Gün**  
Bilgisayar Mühendisliği - ders projesi

