# Otomatik doğrulama kapsamı

`dogrula.sh`, kaynakları ve testlerin tamamını C++17 ile `-Werror` kullanarak temiz derler; ardından birim ve gerçek TCP entegrasyon testlerini çalıştırır. Projede başka bir programlama dili veya ek paket kullanılmaz.

Kontroller: 13 bayt ağ üst bilgisi; standart CRC32 referansı; parçalı soket okuma/yazma; EOF ve SIGPIPE; beş eşzamanlı istemciye benzersiz kimlik atanması; aktif istemci listesinin kendisi hariç dönmesi; Türkçe özel metin, birkaç alıcıya ortak metin, bütün istemcilere toplu metin ve PNG içeriğinin doğru hedeflere yönlendirilmesi; kişinin kendisini hedeflemesinin reddi; altıncı bağlantının kapasite hatasıyla kapatılması ve ayrılan istemcinin yerine yeni bağlantı kabul edilmesi.

Entegrasyon testi yalnız kendi başlattığı sunucu sürecini kapatır ve kişisel alınan dosyaları silmez. Açık başka bir sunucu 5000/TCP kullanıyorsa test hata ile durur.
