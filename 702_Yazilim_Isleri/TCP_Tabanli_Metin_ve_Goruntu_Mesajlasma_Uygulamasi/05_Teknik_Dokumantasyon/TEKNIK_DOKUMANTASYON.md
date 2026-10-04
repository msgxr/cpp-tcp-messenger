# Kurulum, doğrulama ve gösterim

## Windows / WSL2

Ubuntu-20.04 yüklü olmalıdır. Proje klasöründe PowerShell açın:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari/calistir.ps1"
```

Başlatıcı proje yolunu kendi konumundan bulur. Gerekli `g++`, `make`, `python3`, `tmux`, `chafa` ve `iproute2` araçlarını kurar, derler ve iki terminal bölmesi açar. Paket kurulumu internet ve sudo yetkisi gerektirir.

## Linux / WSL terminali

Proje kökünde:

```bash
b="702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari"
bash "$b/kur.sh"
bash "$b/dogrula.sh"
bash "$b/calistir.sh"
```

Doğrulama 36 kontrol içerir. Mevcut gösterimi `cikis` ile kapatın; sonra `dogrula.sh` çalıştırın. GitHub Actions aynı doğrulamayı her push ve pull request için çalıştırır.

## Gösterim

Solda sunucu, sağda istemci bulunur. Sağ bölmede `SELAM`, solda `MERHABA` yazın. Görsel göndermek için iki tarafta da:

```text
!resim 702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/03_Test_ve_Dogrulama_Calismalari/Test_Verileri/piksel_test.png
```

`piksel_test.jpg` de kullanılabilir. Alıcı dosyayı kaydeder, CRC32 doğrular ve renkli piksel bloklarıyla terminalde gösterir. Önizleme ölçeklenir; kaydedilen dosyanın baytları değişmez. Boşluklu bir dosya yolu çift tırnak içine alınabilir.

`!yardim`, `!durum`, `!temizle` ve `cikis` komutları her iki tarafta çalışır. Aynı adlı dosyalar ayrı adlarla kaydedilir. Bağlantı koptuğunda uygulama otomatik kapanır. Alınan dosyalar `04_Uygulama_Ciktilari/Alinan_Dosyalar` altındadır.

## VS Code

Pencerenin sol altında `WSL: Ubuntu-20.04` görünmelidir. `Ctrl+Shift+B` derleme yapar. `Terminal > Run Task` üzerinden `TCP: Tam dogrulama`, `TCP: Iki bolmeli demo`, `TCP: Sunucu` ve `TCP: Istemci` görevleri açılır. Sunucu ve istemci görevleri aynı terminal grubunda gösterilir. Eski Windows VS Code penceresini elle kapatın.
