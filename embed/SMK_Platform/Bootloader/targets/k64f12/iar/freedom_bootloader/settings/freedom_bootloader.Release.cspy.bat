@REM ���̃o�b�`�t�@�C����IAR Embedded WorkbenchC-SPY�f�o�b�K�ɂ����
@REM  �K�؂Ȑݒ���g�p����cspybat�R�}���h���C��
@REM���[�e�B���e�B�����s���邽�߂̃R�}���h���C���̏������x��������̂Ƃ��Đ�������Ă��܂��B
@REM
@REM ���̃t�@�C���͐V�����f�o�b�O�Z�b�V����������������邽�тɐ�������邽�߁A
@REM �t�@�C���𖼏̕ύX�܂��͈ړ����Ă���
@REM �ύX���s�����Ƃ𐄏����܂��B
@REM
@REM cspybat�́A���̃o�b�`�t�@�C�����ɑ����ăf�o�b�O�t�@�C����(�ʏ��ELF/DWARF�܂���UBROF�t�@�C��)����͂��邱�Ƃɂ��A
@REM �N���ł��܂��B
@REM
@REM �g�p�\�ȃR�}���h���C���̃p�����[�^�ɂ��ẮAC-SPY�f�o�b�O�K�C�h���Q�Ƃ��Ă��������B
@REM ����̏ꍇ�ɖ𗧂R�}���h���C���p�����[�^�Ɋւ��鑼�̃q���g
@REM :
@REM   --download_only   ��Ƀf�o�b�O�Z�b�V�������N�������ɃR�[�h�̃C���[�W���_�E�����[�h���܂�
@REM                     �B
@REM   --silent          �T�C���I���̃��b�Z�[�W���ȗ����܂��B
@REM   --timeout         ���s���Ԃ̏����ݒ肵�܂��B
@REM 


"C:\Program Files\IAR Systems\Embedded Workbench 7.0\common\bin\cspybat" "C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\bin\armproc.dll" "C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\bin\armJET.dll"  %1 --plugin "C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\bin\armbat.dll" --device_macro "C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\config\debugger\Freescale\Kxx.dmac" --flash_loader "C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\config\flashloader\Freescale\FlashK64Fxxx128K.board" --backend -B "--endian=little" "--cpu=Cortex-M4F" "--fpu=VFPv4" "-p" "C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\CONFIG\debugger\Freescale\MK64FN1M0xxx12.ddf" "--semihosting=none" "--device=MK64FN1M0xxx12" "--multicore_nr_of_cores=1" "--jet_probe=cmsisdap" "--jet_script_file=C:\Program Files\IAR Systems\Embedded Workbench 7.0\arm\config\debugger\Freescale\KinetisK.ProbeScript" "--jet_standard_reset=5,0,0" "--reset_style="0,-,0,����__���Z�b�g�Ȃ�_"" "--reset_style="1,-,0,�\�t�g�E�F�A"" "--reset_style="2,-,0,�n�[�h�E�F�A"" "--reset_style="3,-,0,�R�A"" "--reset_style="4,-,0,�V�X�e��"" "--reset_style="5,ResetAndDisableWatchdog,1,�J�X�^��"" "--jet_interface=SWD" "--drv_catch_exceptions=0x000" 


