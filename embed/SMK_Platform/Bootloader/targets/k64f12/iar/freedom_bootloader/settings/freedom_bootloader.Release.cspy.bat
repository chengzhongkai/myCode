@REM このバッチファイルはIAR Embedded WorkbenchC-SPYデバッガによって
@REM  適切な設定を使用するcspybatコマンドライン
@REMユーティリティを実行するためのコマンドラインの準備を支援するものとして生成されています。
@REM
@REM このファイルは新しいデバッグセッションが初期化されるたびに生成されるため、
@REM ファイルを名称変更または移動してから
@REM 変更を行うことを推奨します。
@REM
@REM cspybatは、このバッチファイル名に続いてデバッグファイル名(通常はELF/DWARFまたはUBROFファイル)を入力することにより、
@REM 起動できます。
@REM
@REM 使用可能なコマンドラインのパラメータについては、C-SPYデバッグガイドを参照してください。
@REM 特定の場合に役立つコマンドラインパラメータに関する他のヒント
@REM :
@REM   --download_only   後にデバッグセッションを起動せずにコードのイメージをダウンロードします
@REM                     。
@REM   --silent          サインオンのメッセージを省略します。
@REM   --timeout         実行時間の上限を設定します。
@REM 


"C:\Program Files\IAR Systems\Embedded Workbench 7.0\common\bin\cspybat" "C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\bin\armproc.dll" "C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\bin\armJET.dll"  %1 --plugin "C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\bin\armbat.dll" --device_macro "C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\config\debugger\Freescale\Kxx.dmac" --flash_loader "C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\config\flashloader\Freescale\FlashK64Fxxx128K.board" --backend -B "--endian=little" "--cpu=Cortex-M4F" "--fpu=VFPv4" "-p" "C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\CONFIG\debugger\Freescale\MK64FN1M0xxx12.ddf" "--semihosting=none" "--device=MK64FN1M0xxx12" "--multicore_nr_of_cores=1" "--jet_probe=cmsisdap" "--jet_script_file=C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\config\debugger\Freescale\KinetisK.ProbeScript" "--jet_standard_reset=5,0,0" "--reset_style="0,-,0,無効__リセットなし_"" "--reset_style="1,-,0,ソフトウェア"" "--reset_style="2,-,0,ハードウェア"" "--reset_style="3,-,0,コア"" "--reset_style="4,-,0,システム"" "--reset_style="5,ResetAndDisableWatchdog,1,カスタム"" "--jet_interface=SWD" "--drv_catch_exceptions=0x000" 


