$ErrorActionPreference = "Stop"
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
Write-Host "[i] WSL2 / Ubuntu 20.04 başlatılıyor..." -ForegroundColor Cyan
wsl -d Ubuntu-20.04 --cd "$ProjectRoot" bash -lc "chmod +x 702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari/*.sh && 702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/02_Derleme_ve_Calistirma_Dosyalari/calistir.sh"
if ($LASTEXITCODE -ne 0) {
    Write-Host "[X] Çalıştırma başarısız oldu." -ForegroundColor Red
    exit $LASTEXITCODE
}
