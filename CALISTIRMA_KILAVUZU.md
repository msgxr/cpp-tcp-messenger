# Kısa Çalıştırma Kılavuzu

## Gereksinimler

- Windows 11
- WSL2 ve `Ubuntu-20.04`
- İlk kurulum için internet bağlantısı ve `sudo` yetkisi

Uygulama ve testlerin tamamı C++17 ile yazılmıştır.

## Başlatma

PowerShell'i proje klasöründe açın:

```powershell
.\baslat
```

İlk çalıştırmada gerekli Ubuntu paketleri kurulabilir. Ardından proje derlenir ve aynı
ekranda bir sunucu ile beş istemci açılır.

PowerShell proje klasöründe değilse:

```powershell
cd "$HOME\LBLM301 Veri Haberleşmesi ve Bilgisayar Ağları\cpp-tcp-messenger"; .\baslat
```

## Mesaj Gönderme

Önce gönderen istemcinin paneline fareyle tıklayın. Sonra alıcıyı seçin:

```text
3       Yalnız İstemci 3
2,4     İstemci 2 ve 4
0       Gönderen dışındaki herkes
```

Enter'a bastıktan sonra mesajınızı doğrudan yazın:

```text
Merhaba
```

## Görüntü Gönderme

Alıcıları seçtikten sonra:

```text
!resim foto1.jpg
```

JPG, JPEG ve PNG desteklenir. Yalın dosya adları test verileri klasöründe aranır.
Windows Gezgini'nden görüntüyü istemci paneline sürükleyip Enter'a basmak da mümkündür.
Alınan görüntüler `04_Uygulama_Ciktilari/Alinan_Dosyalar` dizinine kaydedilir.

## Diğer Komutlar

```text
!durum      Bağlantı ve seçili alıcıları gösterir
!liste      Aktif istemcileri yeniden listeler
!temizle    İstemci ekranını temizler
!yardim     Kısa yardımı gösterir
cikis       İstemciyi kapatır
```

## Tam Doğrulama

Ubuntu/WSL terminalinde proje kökünden:

```bash
bash 702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari/dogrula.sh
```

Doğrulama; `-Werror` ile temiz derleme, 13 baytlık protokol ve CRC32 birim testleri,
beş istemcili gerçek TCP yönlendirmesi, metin ve görüntü aktarımı, kapasite sınırı ve
ayrılan istemcinin yerine yeni bağlantı kabulünü denetler.
