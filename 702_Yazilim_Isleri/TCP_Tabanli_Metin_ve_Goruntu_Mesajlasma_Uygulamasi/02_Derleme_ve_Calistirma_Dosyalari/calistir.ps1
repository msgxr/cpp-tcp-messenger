param(
    [ValidateRange(2, 5)][int]$IstemciSayisi = 5,
    [string]$Dagitim = "Ubuntu-20.04"
)
$ErrorActionPreference = "Stop"
$ProjeYolu = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
wsl -d $Dagitim --cd "$ProjeYolu" --exec bash "702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari/calistir.sh" $IstemciSayisi
if ($LASTEXITCODE -ne 0) { throw "Başlatma başarısız oldu; yukarıdaki hata mesajını kontrol edin." }
