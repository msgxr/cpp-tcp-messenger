# C++ ile TCP Tabanlı Metin ve Görüntü Mesajlaşma Uygulaması

Bu proje, iki kullanıcı arasında TCP üzerinden çift yönlü metin ve JPG/PNG görüntü aktarımı yapan basit bir istemci-sunucu uygulamasıdır.

## Proje Bilgileri

- Ders projesi: Bilgisayar Mühendisliği
- İşletim sistemi: Ubuntu Linux
- Programlama dili: C++17
- İletişim: TCP
- Varsayılan adres: `127.0.0.1:5000`
- Kullanıcı sayısı: Bir sunucu ve bir istemci

## Özellikler

- İki yönlü metin mesajlaşması
- JPG ve PNG dosyası gönderme
- Görüntüleri `alinanlar/` klasörüne kaydetme
- TCP akışında eksik `send` ve `recv` işlemlerini tamamlama
- Metin ve görüntüyü ayıran uygulama üst bilgisi
- Dosya boyutu ve dosya türü kontrolü
- Mevcut dosyanın üzerine yazmama
- Alım işlemini ayrı bir iş parçacığında yürütme

## Proje Yapısı

```text
tcp_messenger/
├── client.cpp          # İstemci programı
├── server.cpp          # Sunucu programı
├── protocol.h          # Mesaj türleri, üst bilgi ve TCP yardımcıları
├── protocol.cpp        # Proje dosya yapısındaki protokol kaynak dosyası
├── Makefile             # Derleme komutları
├── test_resimleri/      # Test amacıyla kullanılan görüntüler
└── alinanlar/           # Çalışma sırasında alınan görüntüler
```

## Derleme

Proje klasöründe:

```bash
make
```

Temiz derleme için:

```bash
make clean
make
```

## Çalıştırma

İlk terminalde sunucuyu başlatın:

```bash
./server
```

İkinci terminalde istemciyi başlatın:

```bash
./client
```

İstemci bağlandığında şu biçimde mesaj yazılabilir:

```text
Merhaba
```

Programdan çıkmak için:

```text
cikis
```

## Görüntü Gönderme

Test görüntülerinden birini göndermek için:

```text
!resim test_resimleri/foto1.jpg
```

Alınan görüntü karşı tarafta şu klasöre kaydedilir:

```text
alinanlar/foto1.jpg
```

## Mesaj Protokolü

Her mesajın başında 7 baytlık uygulama üst bilgisi bulunur:

| Alan | Boyut | Açıklama |
|---|---:|---|
| Tür | 1 bayt | `1`: metin, `2`: görüntü |
| Dosya adı uzunluğu | 2 bayt | Görüntü dosyasının ad uzunluğu |
| Veri boyutu | 4 bayt | Metin veya görüntü verisinin boyutu |

Sayısal alanlar ağ bayt sırasıyla gönderilir. Önce sabit üst bilgi, ardından görüntü dosya adı ve veri gönderilir. TCP'de tek bir `send` veya `recv` çağrısının tüm veriyi taşıyacağı varsayılmaz.

## Sınırlar

- Metin: en fazla 1 MB
- Görüntü: en fazla 50 MB
- Desteklenen görüntü türleri: JPG ve PNG
- Yerel test adresi: `127.0.0.1`

## Doğrulama

Uygulama aşağıdaki temel senaryolarla test edilmiştir:

- Kısa ve Türkçe metin gönderimi
- İstemciden sunucuya görüntü gönderimi
- Sunucudan istemciye görüntü gönderimi
- Ardışık görüntü gönderimi
- Alınan görüntünün VS Code görüntüleyicisinde açılması
- Aynı isimli dosyanın üzerine yazılmasının engellenmesi

## Kapsam Dışı

Bu çalışmada grup sohbeti, kullanıcı hesabı, mesaj geçmişi, grafik arayüz, şifreleme ve internet üzerinden güvenli iletişim bulunmamaktadır. Uygulama aynı bilgisayardaki yerel TCP testleri için hazırlanmıştır.

## Lisans

Bu proje ders ödevi kapsamında hazırlanmıştır.
