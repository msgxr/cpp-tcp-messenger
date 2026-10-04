# Otomatik doğrulama kapsamı

`dogrula.sh`, kaynakları C++17 ve `-Werror` ile temiz derler; ardından 36 kontrol çalıştırır. Ana uygulama C++17, test sürücüsü Python 3.8 veya üzeridir. Python paketi veya pip kurulumu gerekmez.

Kontroller: 11 bayt ağ üst bilgisi; standart CRC32 referansı; parçalı soket okuma/yazma; EOF ve SIGPIPE; gerçek sunucu–istemci arasında iki yönlü Türkçe metin; uzun metnin tamamı; yardım/durum/temizle komutları; PNG/JPG dosyalarının iki yönlü aktarımı; ANSI renkli piksel önizlemesi; eşzamanlı aynı adlı dosyalar; boşluklu ve tırnaklı yollar; olmayan/boş/uygunsuz dosyalar; 1 MiB metin ve 50 MiB görsel sınırları; sınır aşımının reddi; parçalı ve art arda TCP paketleri; hatalı CRC32; yarım aktarımda dosya temizliği; geçersiz mesaj türü/başlık/uzantı; dosya adında üst dizine çıkış; normal ve ani bağlantı kapanışı; sunucu yokken hata; CRLF girdisi; gerçek tmux başlatıcısında metin/görsel aktarımı.

Her çalıştırma `Test_Sonuclari/SON_TEST_RAPORU.md`, `.json` ve `uygulama.log` üretir. Bu geçici çıktılar Git'e eklenmez. Testler yalnız kendi alt süreçlerini ve ayrı tmux test oturumunu kapatır; kişisel alınan dosyaları silmez. Açık başka bir sunucu 5000/TCP kullanıyorsa test açık bir hata ile durur.

