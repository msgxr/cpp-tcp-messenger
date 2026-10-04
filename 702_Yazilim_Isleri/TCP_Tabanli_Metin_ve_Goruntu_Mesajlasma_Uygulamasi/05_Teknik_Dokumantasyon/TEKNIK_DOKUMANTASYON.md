# Proje Kurulum, Doğrulama ve Gösterim Kılavuzu

Bu rehber, projenin **Windows (WSL2)** ve **Yerel Linux** ortamlarında hiçbir hata ("patlama") almadan, sıfırdan sorunsuz bir şekilde ayağa kaldırılması, doğrulanması ve çalıştırılması için **en ince detayına kadar** hazırlanmıştır.

---

## 🛠️ 1. Ortam Gereksinimleri ve Ön Hazırlık

Çalıştırma aşamasına geçmeden önce sisteminizin aşağıdaki temel şartları sağladığından emin olun:
* **Windows Kullanıcıları İçin:** Sisteminizde **WSL2** ve **Ubuntu-20.04** dağıtımı kurulu ve varsayılan olarak ayarlanmış olmalıdır.
* **İnternet Bağlantısı:** İlk çalıştırmada sistem paketleri (`apt`) güncelleneceği için aktif bir internet bağlantısı şarttır.
* **Sudo Yetkisi:** Kurulum betikleri (`kur.sh`), eksik paketleri yüklemek için sizden `sudo` (root) şifrenizi isteyecektir. Şifrenizi girmeye hazır olun.

---

## 💻 2. Windows Sistemlerde (CMD / PowerShell) Bash Nasıl Çalışır?

Proje klasöründeki `calistir.ps1` betiğini veya Windows terminallerini kullanırken, arka planda Windows ve Linux (WSL2) katmanları arasında şu köprü mekanizması işler:

* **Sanal Terminal Köprüsü (`wsl.exe`):** Windows CMD veya PowerShell üzerinden doğrudan Linux komutları koşturulamaz. Bu nedenle sistem, Windows tabanlı çalışan `wsl.exe` aracını bir köprü olarak kullanır.
* **Komutun Linux'a Aktarılması:** Siz PowerShell üzerinde betiği tetiklediğinizde, PowerShell bu isteği `wsl -d Ubuntu-20.04 -e bash -c "komutlar"` yapısına dönüştürür. Buradaki `-d` parametresi hedef Linux dağıtımını (Ubuntu-20.04) belirtirken, `-e` parametresi ise Linux içindeki `bash` kabuğunu çalıştırıp komutu oraya paslar.
* **Dosya Sistemi Eşleştirmesi (`/mnt/c`):** Windows üzerindeki proje klasörünüz (`C:\Users\...`), WSL2 içerisine `/mnt/c/Users/...` yoluyla otomatik olarak bağlanır (mount edilir). Betik çalıştırıldığında proje yollarının patlamamasının sebebi, `calistir.ps1` dosyasının bu Windows yolunu otomatik olarak Linux'un anlayacağı dinamik `/mnt/` yol formatına çevirmesidir.
* **İzin ve Bypass Politikası:** PowerShell'in varsayılan güvenlik politikaları dışarıdan betik çalıştırılmasını engelleyebilir. Bu yüzden `calistir.ps1` tetiklenirken kullanılan `-ExecutionPolicy Bypass` parametresi, Windows güvenlik duvarına takılmadan Linux alt yapısının sorunsuz başlamasını sağlar.

---

## 🚀 3. Kurulum ve Çalıştırma Yöntemleri

Geliştirme yaptığınız ortama göre aşağıdaki **Yöntem A** veya **Yöntem B** seçeneklerinden birini seçerek ilerleyin.

### Yöntem A: Windows Üzerinden PowerShell ile (Otomatik Yöntem)
Eğer Linux terminaline hiç girmeden doğrudan Windows PowerShell üzerinden projeyi ayağa kaldırmak istiyorsanız bu adımı uygulayın.

1. Projenin **ana (kök) klasöründe** bir PowerShell penceresi açın.
2. Aşağıdaki komutu kopyalayın ve sağ tıklayarak PowerShell'e yapıştırıp `Enter` tuşuna basın:
   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File "702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari/calistir.ps1"
   ```
   * **Arka Planda Ne Oluyor?** Bu betik, yukarıda açıklanan `wsl.exe` köprüsünü kurarak Ubuntu-20.04 ortamına bağlanır. Eksik olan `g++`, `make`, `python3`, `tmux`, `chafa` ve `iproute2` paketlerini `apt` aracılığıyla otomatik kurar. Kaynak kodları derler ve terminali ikiye bölerek demoyu başlatır.

### Yöntem B: Doğrudan Linux / WSL Terminali İçinden (Manuel/Kontrollü Yöntem)
Eğer zaten Ubuntu/WSL terminalinin içerisine girdiyseniz ve adımları tek tek görerek çalıştırmak istiyorsanız bu yöntemi izleyin.

1. Terminalde projenin **kök dizinine** (`cd /proje/yolu`) gidin.
2. Kolaylık olması için betik yolunu bir değişkene atayın:
   ```bash
   b="702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari"
   ```
3. **Bağımlılıkları Kurun:** Sistemde eksik araç kalmaması için kurulum betiğini çalıştırın (Sudo şifresi isteyebilir):
   ```bash
   bash "$b/kur.sh"
   ```
4. **Projeyi Derleyin ve Çalıştırın:** `tmux` tabanlı ikili demo ekranını açmak için:
   ```bash
   bash "$b/calistir.sh"
   ```

---

## 🧪 4. Sistem Doğrulaması (Testlerin Koşturulması)

Uygulamanın protokol seviyesinde, bellek yönetiminde veya dosya aktarımında bir sorun olup olmadığını test etmek için **36 farklı kontrol içeren** otomatik bir doğrulama mekanizması entegre edilmiştir. GitHub Actions (CI/CD) hattı da her kod gönderiminde bu testi baz alır.

1. Eğer ekranda açık bir demo (`calistir.sh` veya `calistir.ps1` ile açılmış ekran) varsa, önce terminal bölmelerine `cikis` yazarak demoyu güvenli bir şekilde kapatın.
2. Proje kök dizininde şu komutu çalıştırarak testleri başlatın:
   ```bash
   bash "702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari/dogrula.sh"
   ```
3. Ekranda 36 kontrolün tamamının yeşil veya başarılı şekilde sonuçlandığını görün. Herhangi bir "patlama" veya hata durumunda bu betik size detaylı hata logu basacaktır.

---

## 💻 5. Canlı Gösterim ve Kullanım Senaryoları

Betik başarıyla çalıştığında karşınıza `tmux` ile **yatay veya dikey olarak ikiye bölünmüş** bir terminal gelecektir:
* **Sol Bölme:** TCP Sunucu (Server) rolündedir.
* **Sağ Bölme:** TCP İstemci (Client) rolündedir.

### Metin Mesajlaşması Testi
1. Sağdaki istemci terminaline tıklayın ve klavyeden `SELAM` yazıp `Enter`'a basın. Mesajın sol tarafa anında düştüğünü doğrulayın.
2. Soldaki sunucu terminaline geçiş yapın, `MERHABA` yazıp `Enter`'a basın. Mesajın sağ tarafa ulaştığını görün.

### Görsel Gönderimi ve Terminalde Önizleme Testi
Uygulama, gönderilen resimlerin bütünlüğünü **CRC32** algoritması ile doğrular ve alıcı tarafın terminalinde `chafa` aracı sayesinde **renkli piksel blokları** halinde görselleştirir.

1. İki taraftan herhangi birinde şu komutu yapıştırarak test görselini gönderin:
   ```text
   !resim 702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/03_Test_ve_Dogrulama_Calismalari/Test_Verileri/piksel_test.png
   ```
   *(Dilerseniz aynı dizindeki `piksel_test.jpg` dosyasını da deneyebilirsiniz.)*
2. **Önemli Detaylar:**
   * Eğer göndermek istediğiniz dosya yolunda boşluk karakteri varsa, yolu çift tırnak içine almalısınız: `!resim "C:/Klasor Yolu/resim.png"` gibi.
   * **Bütünlük Garantisi:** Terminaldeki görsel sadece bir önizlemedir ve terminal boyutuna göre ölçeklenir. Alıcının diskine kaydedilen asıl dosyanın bayt boyutu ve kalitesi **asla değişmez**, orijinaliyle birebir aynı kalır.
   * **İsim Çakışması Önleme:** Eğer aynı isimde bir dosyayı üst üste gönderirseniz, sistem eskisinin üzerine yazmaz; dosyayı `piksel_test_1.png`, `piksel_test_2.png` gibi benzersiz isimlerle saklar.
   * **Kayıt Yeri:** Gelen tüm dosyalar güvenli bir şekilde `04_Uygulama_Ciktilari/Alinan_Dosyalar` klasörü altında depolanır.

### Kullanışlı Konsol Komutları
Her iki terminal bölmesinde de kullanabileceğiniz fonksiyonel komutlar:
* `!yardim` : Kullanılabilecek komutların listesini ve açıklamalarını basar.
* `!durum` : Mevcut TCP bağlantısının aktiflik durumunu ve soket bilgilerini gösterir.
* `!temizle` : Terminal ekranındaki eski mesaj yoğunluğunu temizleyerek net bir ekran sunar.
* `cikis` : Uygulamayı güvenli bir şekilde kapatır. Soketleri serbest bırakır.

> ⚠️ **Kritik Davranış Notu:** TCP bağlantısı herhangi bir sebepten ötürü (kablo çekilmesi, uygulamanın çökmesi, karşı tarafın aniden kapanması) koptuğu anda, sistem bunu anında algılar ve kaynakları sömürmemek adına kendisini **otomatik olarak güvenli modda kapatır**.

---

## ⚙️ 6. VS Code Entegrasyonu ve Görevler (Tasks)

Projeyi tamamen VS Code içerisinden yönetmek ve terminal komutlarıyla uğraşmamak istiyorsanız şu kurallara dikkat edin:

1. **Doğru Ortam:** VS Code penceresini açtığınızda, sol alt köşedeki yeşil bağlantı motorunda mutlaka **`WSL: Ubuntu-20.04`** ibaresini görmelisiniz. Eğer Windows modundaysanız projeyi WSL üzerinde aç seçeneğini kullanın.
2. **Klavye Kısayolu:** Kodda bir değişiklik yaptıktan sonra derlemek için direkt **`Ctrl+Shift+B`** kombinasyonunu kullanabilirsiniz. Bu kombinasyon varsayılan derleme görevini tetikler.
3. **Hazır Görevler (Tasks):** Üst menüden `Terminal > Run Task...` (Görev Çalıştır) sekmesine tıkladığınızda karşınıza şu hazır senaryolar gelecektir:
   * `TCP: Tam dogrulama` -> 36 maddelik otomatik test protokolünü çalıştırır.
   * `TCP: Iki bolmeli demo` -> Terminali tmux gibi bölerek hazır sunucu-istemci demosunu açar.
   * `TCP: Sunucu` -> Sadece sunucu uygulamasını bağımsız bir terminalde kaldırır.
   * `TCP: Istemci` -> Sadece istemci uygulamasını bağımsız bir terminalde kaldırır.
   
   *Not: `TCP: Sunucu` ve `TCP: Istemci` görevlerini çalıştırdığınızda, VS Code bunları karmaşıklığı önlemek adına **aynı terminal panel grubunda** sekmeli olarak gösterir.*
4. **Çakışma Uyarısı:** Eğer arkada önceden kalma, askıda kalmış eski bir Windows VS Code penceresi veya terminal oturumu varsa, soketlerin (`port`) çakışmaması ve uygulamanın patlamaması için o eski pencereleri **elle kapatın**.
