# C++ ile TCP Tabanlı Metin ve Görüntü Mesajlaşma Uygulaması

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B&logoColor=white) ![Platform](https://img.shields.io/badge/Platform-Windows%2011%20%2B%20WSL2-0078D4?logo=windows) ![Ubuntu](https://img.shields.io/badge/Ubuntu-20.04%20LTS-E95420?logo=ubuntu&logoColor=white) ![Protocol](https://img.shields.io/badge/Protocol-TCP-2ea44f) ![Integrity](https://img.shields.io/badge/Integrity-CRC32-orange) ![Standard](https://img.shields.io/badge/Standard-C%2B%2B17-blue)

Bu proje, **TCP soket programlama**, **uygulama katmanı mesaj çerçeveleme**, **metin ve ikili dosya aktarımı**, **veri bütünlüğü doğrulaması** ve **eşzamanlı haberleşme** konularını uygulamalı olarak göstermek amacıyla C++17 kullanılarak geliştirilmiştir.

Uygulama, Windows 11 üzerinde **WSL2 / Ubuntu 20.04 LTS** ortamında çalışır. Sunucu ve istemci aynı TCP bağlantısı üzerinden çift yönlü olarak metin mesajı ve JPG/JPEG/PNG biçimindeki görüntü dosyalarını aktarabilir.

> `tmux` ve `chafa` yalnızca terminal sunumu ve görüntü önizlemesi için kullanılan yardımcı araçlardır. TCP haberleşme katmanı C++17 ile gerçekleştirilmiştir.

---

## 1. Projenin Amacı

Projenin temel amacı, TCP'nin güvenilir ve sıralı bayt akışı yapısı üzerinde uygulama seviyesinde bir mesajlaşma protokolü oluşturmaktır. Bu kapsamda sistem aşağıdaki işlevleri gerçekleştirir:

- İstemci ile sunucu arasında TCP bağlantısı kurulması
- Çift yönlü metin mesajı aktarımı
- JPG, JPEG ve PNG görüntü aktarımı
- Mesaj türü ve veri boyutunun özel uygulama başlığı ile taşınması
- Kısmi `send()` ve `recv()` işlemlerinin güvenli biçimde tamamlanması
- Metin ve görüntüler için CRC32 bütünlük doğrulaması
- Gelen görüntülerin güvenli dosya adıyla kaydedilmesi
- Aynı isimli dosyaların üzerine yazılmasının önlenmesi
- Gönderim ve alım ilerlemesinin terminalde gösterilmesi
- Gelen görüntünün `chafa` ile terminal içinde önizlenmesi
- `tmux` ile sunucu ve istemcinin aynı ekranda iki ayrı panelde çalıştırılması

---

## 2. Sistem Mimarisi

Uygulama klasik istemci-sunucu mimarisini kullanır.

```text
┌──────────────────────┐          TCP / IPv4          ┌──────────────────────┐
│      TCP İstemci     │ <──────────────────────────> │      TCP Sunucu      │
│  127.0.0.1:dinamik   │       127.0.0.1:5000         │    0.0.0.0:5000      │
└──────────────────────┘                              └──────────────────────┘
          │                                                       │
          ├── Metin gönder / al                                   ├── Metin gönder / al
          ├── Görüntü gönder / al                                 ├── Görüntü gönder / al
          ├── CRC32 doğrulama                                     ├── CRC32 doğrulama
          └── Alıcı iş parçacığı                                  └── Alıcı iş parçacığı
```

Sunucu `5000/TCP` portunu dinler. İstemci varsayılan olarak `127.0.0.1:5000` adresine bağlanır. Bağlantı kurulduktan sonra her iki taraf da gönderici ve alıcı olarak çalışabilir.

---

## 3. Uygulama Protokolü

TCP mesaj sınırlarını kendiliğinden belirlemez; yalnızca sıralı bir bayt akışı sağlar. Bu nedenle proje, her mesajın başına **11 baytlık sabit bir uygulama üst bilgisi** ekler.

| Alan | Boyut | Açıklama |
|---|---:|---|
| `type` | 1 bayt | `1`: metin, `2`: görüntü |
| `filename_len` | 2 bayt | Dosya adı uzunluğu; metinde `0` |
| `data_size` | 4 bayt | Gönderilen içeriğin bayt cinsinden boyutu |
| `checksum` | 4 bayt | İçeriğin CRC32 doğrulama değeri |
| **Toplam** | **11 bayt** | Sabit uygulama başlığı |

Sayısal alanlar ağ bayt sırasına dönüştürülür. Bunun için `htons()`, `htonl()`, `ntohs()` ve `ntohl()` fonksiyonları kullanılır.

Metin mesajının yapısı:

```text
[ 11 bayt başlık ][ metin verisi ]
```

Görüntü mesajının yapısı:

```text
[ 11 bayt başlık ][ dosya adı ][ görüntü verisi ]
```

---

## 4. Veri Bütünlüğü ve Aktarım Güvenilirliği

TCP güvenilir aktarım sağlar ancak tek bir `send()` veya `recv()` çağrısının istenen tüm baytları aynı anda işleyeceği garanti edilmez. Bu nedenle projede:

```cpp
send_exact(...)
read_exact(...)
```

fonksiyonları kullanılır. Bu fonksiyonlar, gerekli bayt sayısının tamamı gönderilene veya alınana kadar işlemi döngü içerisinde sürdürür.

Metin ve görüntü içerikleri ayrıca **CRC32** ile doğrulanır. Gönderici CRC32 değerini hesaplayarak mesaj başlığına ekler. Alıcı aynı verinin CRC32 değerini yeniden hesaplar ve iki değeri karşılaştırır.

> CRC32 bir kriptografik güvenlik veya kimlik doğrulama mekanizması değildir. Bu projede yalnızca aktarım bütünlüğünü denetlemek amacıyla kullanılmaktadır.

---

## 5. Kurumsal Dizin Yapısı

Proje dizini, kaynak kod, derleme dosyaları, test verileri, çalışma çıktıları ve proje belgelerinin birbirinden ayrılması amacıyla kurumsal dosyalama yaklaşımına göre düzenlenmiştir.

```text
cpp-tcp-messenger/
│
├── 702_Yazilim_Isleri/
│   └── TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/
│       │
│       ├── 01_Kaynak_Kod_Dosyalari/
│       │   ├── client.cpp
│       │   ├── server.cpp
│       │   ├── protocol.cpp
│       │   ├── protocol.h
│       │   ├── chat_common.h
│       │   └── ui.h
│       │
│       ├── 02_Derleme_ve_Calistirma_Dosyalari/
│       │   ├── Makefile
│       │   ├── calistir.ps1
│       │   ├── calistir.sh
│       │   ├── istemci.sh
│       │   ├── sunucu.sh
│       │   └── Derlenmis_Uygulama_Dosyalari/
│       │       ├── client
│       │       └── server
│       │
│       ├── 03_Test_ve_Dogrulama_Calismalari/
│       │   └── Test_Verileri/
│       │
│       ├── 04_Uygulama_Ciktilari/
│       │   └── Alinan_Dosyalar/
│       │
│       └── 05_Teknik_Dokumantasyon/
│
├── 703_Bilgi_Isletim_Sistemleri_Planlama_ve_Degerlendirme/
│   ├── 01_Proje_Yonetimi/
│   │   └── Proje_Raporlari/
│   │       └── LBLM301_Proje_Raporu.tex
│   │
│   └── 02_Degisiklik_ve_Konfigurasyon_Yonetimi/
│       └── DEGISIKLIK_KAYDI.md
│
├── README.md
└── .gitignore
```

`Derlenmis_Uygulama_Dosyalari/` ve çalışma sırasında oluşturulan alınan dosyalar Git deposuna dahil edilmez.

---

## 6. Kaynak Kod Bileşenleri

| Dosya | Sorumluluk |
|---|---|
| `client.cpp` | TCP istemcisini oluşturur, sunucuya bağlanır ve kullanıcı girişlerini işler. |
| `server.cpp` | TCP sunucusunu oluşturur, `5000` portunu dinler ve istemci bağlantısını kabul eder. |
| `protocol.h` | Mesaj türlerini, 11 baytlık başlığı, boyut sınırlarını ve protokol fonksiyon bildirimlerini tanımlar. |
| `protocol.cpp` | `send_exact()`, `read_exact()` ve CRC32 işlemlerinin uygulamasını içerir. |
| `chat_common.h` | Metin/görüntü gönderme ve alma, dosya doğrulama, güvenli kayıt ve ortak haberleşme mantığını içerir. |
| `ui.h` | Terminal renkleri, bilgi kartları, ilerleme çubuğu, yardım ekranı ve `chafa` önizlemesini yönetir. |
| `Makefile` | Sunucu ve istemci uygulamalarını C++17 ile derler. |
| `calistir.sh` | Derleme ve `tmux` tabanlı çift panel çalışma sürecini otomatikleştirir. |
| `calistir.ps1` | Windows PowerShell üzerinden WSL2 çalışma sürecini başlatır. |
| `sunucu.sh` | Derlenmiş sunucu uygulamasını başlatır. |
| `istemci.sh` | Derlenmiş istemci uygulamasını başlatır. |

---

## 7. Çalışma Ortamı

| Bileşen | Kullanılan Teknoloji |
|---|---|
| Ana işletim sistemi | Windows 11 |
| Linux ortamı | WSL2 / Ubuntu 20.04 LTS |
| Programlama dili | C++17 |
| Derleyici | GNU `g++` |
| Derleme sistemi | GNU Make |
| Ağ protokolü | TCP / IPv4 |
| Varsayılan port | `5000/TCP` |
| İş parçacığı | `std::thread` / `pthread` |
| Terminal oturumu | `tmux` |
| Görüntü önizleme | `chafa` |
| Hata ayıklama | `gdb` |

---

## 8. Gereksinimler

Ubuntu 20.04 ortamında temel geliştirme araçları:

```bash
sudo apt update
sudo apt install -y build-essential gdb tmux chafa
```

WSL2 dağıtımının adı aşağıdaki komutla kontrol edilebilir:

```powershell
wsl -l -v
```

Bu proje için kullanılan dağıtım:

```text
Ubuntu-20.04
```

---

## 9. Derleme

Proje kök dizininden:

```bash
make -C 702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari clean
make -C 702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari
```

Derleme seçenekleri:

```text
-Wall -Wextra -Wpedantic -std=c++17 -pthread
```

Başarılı derleme sonucunda aşağıdaki dosyalar oluşturulur:

```text
02_Derleme_ve_Calistirma_Dosyalari/
└── Derlenmis_Uygulama_Dosyalari/
    ├── server
    └── client
```

---

## 10. Otomatik Çalıştırma

### Windows PowerShell üzerinden

Proje kök dizininde:

```powershell
wsl -d Ubuntu-20.04 --cd "$PWD" bash -lc "chmod +x 702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari/*.sh && 702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari/calistir.sh"
```

Bu işlem projeyi derler ve `tmux` üzerinde sunucu ile istemciyi iki ayrı panelde açar.

### WSL / Ubuntu üzerinden

```bash
./702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari/calistir.sh
```

---

## 11. Manuel Çalıştırma

Önce sunucu:

```bash
./702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari/Derlenmis_Uygulama_Dosyalari/server
```

Daha sonra ikinci terminalde istemci:

```bash
./702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari/Derlenmis_Uygulama_Dosyalari/client
```

---

## 12. Kullanım

Normal metin göndermek için terminale doğrudan mesaj yazılır:

```text
Merhaba TCP
```

Test görüntüsü göndermek için:

```text
!resim foto1.jpg
```

Desteklenen komutlar:

| Komut | İşlev |
|---|---|
| `!resim <dosya_adı veya dosya_yolu>` | JPG/JPEG/PNG görüntü gönderir. Yalın adlar test klasöründe aranır. |
| `!durum` | Aktif bağlantı durumunu gösterir. |
| `!temizle` | Terminal ekranını temizler. |
| `!yardim` | Kullanılabilir komutları gösterir. |
| `cikis` | TCP bağlantısını düzenli biçimde kapatır. |

---

## 13. Görüntü Aktarım Akışı

Görüntü gönderilirken uygulama aşağıdaki sırayı izler:

```text
Dosya yolu kontrolü
↓
JPG/JPEG/PNG uzantı kontrolü
↓
Dosya boyutu kontrolü
↓
CRC32 hesaplama
↓
11 baytlık başlığın hazırlanması
↓
Dosya adının gönderilmesi
↓
8192 baytlık parçalar hâlinde görüntü aktarımı
↓
Alıcıda yeniden CRC32 hesaplama
↓
CRC32 eşleşirse dosyanın kabul edilmesi
↓
Terminalde chafa önizlemesi
```

Gelen dosyalar aşağıdaki klasöre kaydedilir:

```text
702_Yazilim_Isleri/
└── TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/
    └── 04_Uygulama_Ciktilari/
        └── Alinan_Dosyalar/
```

---

## 14. Güvenlik ve Doğrulama Kontrolleri

Uygulamada aşağıdaki koruyucu kontroller bulunmaktadır:

- Metin boyutu en fazla **1 MB**
- Görüntü boyutu en fazla **50 MB**
- Yalnız JPG/JPEG/PNG uzantılarının kabul edilmesi
- Gelen dosya adının `safe_filename()` ile temizlenmesi
- Dizin dışına yazmaya yönelik dosya yollarının engellenmesi
- Aynı isimli mevcut dosyanın üzerine yazılmaması
- Eksik TCP aktarımının başarısız kabul edilmesi
- CRC32 uyuşmazlığında bozuk görüntünün silinmesi
- Bilinmeyen mesaj türlerinin reddedilmesi
- Geçersiz başlık alanlarının kontrol edilmesi

---

## 15. Eşzamanlılık Tasarımı

Her iki uygulamada da gelen verilerin kullanıcı girişinden bağımsız olarak alınabilmesi için ayrı bir alıcı iş parçacığı oluşturulur:

```cpp
std::thread receiver(receive_loop, socket_fd);
```

Böylece ana iş parçacığı terminalden kullanıcı girdisini beklerken alıcı iş parçacığı karşı taraftan gelen TCP verisini işlemeye devam eder.

Terminal çıktılarının farklı iş parçacıkları tarafından aynı anda bozulmaması için `ui.h` içerisinde `std::mutex` kullanılır.

---

## 16. Teknik Tasarım Kararları

**Neden TCP?** Metin ve dosya aktarımında sıralı, güvenilir ve bağlantı tabanlı veri iletimi gerektiği için TCP tercih edilmiştir.

**Neden 11 baytlık özel başlık?** TCP mesaj sınırlarını bilmediğinden alıcının mesaj türünü, dosya adı uzunluğunu, veri boyutunu ve CRC32 değerini önceden bilmesi gerekir.

**Neden `send_exact()` ve `read_exact()`?** Soket API'sinde tek bir `send()` veya `recv()` çağrısı istenen tüm veriyi işlemeyebilir. Yardımcı fonksiyonlar, aktarım tamamlanana kadar işlemi sürdürür.

**Neden ağ bayt sırası?** Çok baytlı sayısal alanların farklı sistem mimarilerinde aynı biçimde yorumlanmasını sağlar.

**Neden CRC32?** Aktarılan verinin gönderici ve alıcı tarafında aynı olup olmadığını kontrol etmek için kullanılır.

**Neden 8192 baytlık tampon?** Büyük görüntü dosyalarının tamamını tek seferde belleğe almak yerine parçalı ve kontrollü aktarım yapılmasını sağlar.

**Neden ayrı alıcı iş parçacığı?** Kullanıcı klavye girişi beklerken gelen mesajların bloklanmaması için kullanılır.

**Neden `SO_REUSEADDR`?** Sunucu yeniden başlatıldığında TCP portunun gereksiz yere kullanılamaz durumda kalması ihtimalini azaltır.

**Neden `shutdown()` ve `close()`?** Bağlantının kontrollü biçimde sonlandırılması ve soket kaynaklarının işletim sistemine geri verilmesi için kullanılır.

---

## 17. Test ve Doğrulama Senaryoları

| Test | Beklenen Sonuç |
|---|---|
| Kısa metin aktarımı | Metin eksiksiz alınır. |
| Türkçe karakterli metin | İçerik bozulmadan alınır. |
| JPG aktarımı | Görüntü kaydedilir ve CRC32 doğrulanır. |
| PNG aktarımı | Görüntü kaydedilir ve CRC32 doğrulanır. |
| Ardışık mesajlar | Mesaj sınırları birbirine karışmaz. |
| Aynı isimli görüntü | Yeni ve benzersiz dosya adı oluşturulur. |
| Geçersiz dosya yolu | Gönderim başlatılmaz ve hata gösterilir. |
| Desteklenmeyen uzantı | Dosya reddedilir. |
| Boyut sınırı aşımı | Aktarım başlatılmaz. |
| CRC32 uyuşmazlığı | Bozuk veri kabul edilmez. |
| Bağlantı kesilmesi | Eksik aktarım başarılı sayılmaz. |
| Düzenli çıkış | Soket ve iş parçacığı kontrollü biçimde kapatılır. |

---

## 18. Proje Kapsamı ve Sınırlamalar

Bu sürüm eğitim ve yerel ağ haberleşmesi senaryosu için hazırlanmıştır. Mevcut kapsam:

```text
1 sunucu
1 istemci
TCP / IPv4
Metin mesajı
JPG / JPEG / PNG görüntü aktarımı
CRC32 bütünlük kontrolü
Yerel çalışma ortamı
```

Aşağıdaki özellikler mevcut sürümün kapsamı dışındadır:

```text
Çoklu istemci yönetimi
Kullanıcı hesabı ve oturum sistemi
Veritabanı
Kalıcı mesaj geçmişi
TLS
Uçtan uca şifreleme
Kriptografik kimlik doğrulama
İnternet üzerinden üretim ortamı kullanımı
```

---

## 19. Git ve Derleme Politikası

Derlenmiş programlar ve çalışma çıktıları kaynak kod deposunda tutulmaz. `.gitignore` üzerinden aşağıdaki içerikler hariç tutulur:

```text
Derlenmis_Uygulama_Dosyalari/
Alinan_Dosyalar/*
*.o
*.gch
*.aux
*.log
*.out
*.toc
.vscode/
```

Kaynak kod değişiklikleri Git commit geçmişi üzerinden izlenir. Dosya adlarında `SON`, `FINAL`, `YENI` veya `GUNCEL` gibi manuel sürüm ifadeleri yerine sürüm kontrol sistemi kullanılması hedeflenmiştir.

---

## 20. Proje Sahibi

**Muhammed Sina Gün** Bilgisayar Mühendisliği LBLM301 Veri Haberleşmesi ve Bilgisayar Ağları Ders Projesi

---

## 21. Lisans ve Kullanım

Bu depo akademik ders projesi kapsamında hazırlanmıştır. Kodun yeniden kullanımı veya geliştirilmesi durumunda proje kaynağının belirtilmesi önerilir.

<!-- TCP_HAZIRLIK_BEGIN -->
## Otomatik doğrulama ve gösterim

Temiz derleme, 36 kontrol, iki yönlü PNG/JPG aktarımı, renkli piksel önizlemesi ve VS Code görevleri hazırdır. Güncel başlangıç adımları [Çalıştırma Kılavuzu](CALISTIRMA_KILAVUZU.md), ayrıntılı test kapsamı [Test Kapsamı](702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/03_Test_ve_Dogrulama_Calismalari/TEST_KAPSAMI.md) dosyasındadır. GitHub Actions her gönderimde aynı doğrulamayı çalıştırır.

WSL/Linux terminalinde proje kökünden:

```bash
b="702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari"
bash "$b/dogrula.sh"
bash "$b/calistir.sh"
```

Gösterimde istemciye `!resim piksel_test.png` yazın. Yalın dosya adları otomatik olarak
`03_Test_ve_Dogrulama_Calismalari/Test_Verileri` klasöründe aranır. Windows Gezgini'nden
sürüklenen tırnaklı yollar temizlenir ve `C:\...` yolları WSL biçimine çevrilir. Alıcı
CRC32 doğrular, dosyayı kaydeder ve renkli piksel bloklarıyla gösterir.
Görseli Windows Gezgini'nden terminale sürükleyip yalnızca Enter'a basmak da yeterlidir;
JPG/JPEG/PNG yolu otomatik olarak görsel aktarımı kabul edilir.
<!-- TCP_HAZIRLIK_END -->
