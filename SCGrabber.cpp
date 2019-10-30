#include "stdafx.h"

typedef VOID (*MYPROC)(LPTSTR);

// ��� ���������� ������ �������� ������������ ������������ �� 1 ����!
// ��������� ���� ��������� ����� ������� ���� � 16-������ ��������� WinHex
// ��������� ��������� ���� ���������
struct SCDBinfo
{
   __int32 par1;		// 535353
   __int32 par2;		// 1
   __int32 par3;		// ������ �� ������
   __int32 par4;		// 256
   __int32 par5;		// ������ �� ���������� ��������� ������ ������ � ����
   __int32 count;		// ���������� ������� � ����
   __int64 par7;		// ���� 
   __int16 format;	// ������ ������� (WAVE_FORMAT_PCM 0x0001)
   __int16 nchans;	// ���-�� ������� (1)
   __int32 srate;		// ������� ������������� (12000)
   __int32 bitrate;	// �������� ���� � ��� (24000)
   __int16 nbytes;	// ���-�� ���� � ������ (1)
   __int16 nbits;		// ���-�� ��� � ������ (8)
};

// ��������� ��������� ��������� ������ ���������
struct SCRECinfo
{
   __int32 adrCurr;	// ���������� ��������� ������� ������ � ����
   __int32 adrPrev;	// ���������� ��������� ���������� ������ � ����
   __int32 lenght;	// ����� �������� ������ � ������
   __int32 par4;		// 1
   __int8 par5[10];	// ����
   __int32 index;		// ������ ������ � ����
   __int32 par7;		// 2
   __int32 freq;		// �������� ������� (MMMKKKHHH)
   __int32 datatime;	// UTC - ���-�� ��� ��������� � 0:00:00 1 ������ 1970
   __int8 par10[18];	// ����
   __int8 par11[3];	// �/� �����
   char radio[20];	// ��� ��������
   char info[166];	// ���������� � ������
};

// ��������� ��������� ��������� ����� ������� WAVE
struct WAVEinfo
{
   __int32 triff;		// ����� "RIFF" (1179011410)
   __int32 lenght;	// ������ ����� - 8 ����
   __int32 twave;		// ����� "WAVE" (1163280727)
   __int32 tfmt;		// ����� "fmt " (544501094)
   __int32 n16;		// 16
   __int16 format;	// ������ (WAVE_FORMAT_PCM 0x0001)
   __int16 nchans;	// ���-�� ������� (1)
   __int32 srate;		// ������� ������������� (12000)
   __int32 bitrate;	// �������� ���� � ��� (24000)
   __int16 nbytes;	// ���-�� ���� � ������ (1)
   __int16 nbits;		// ���-�� ��� � ������ (8)
   __int32 tdata;		// ����� "data" (1635017060)
   __int32 ndata;		// ���-�� ���� � ������� ������
};

DWORD CALLBACK CopyProgressRoutine(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred,
                                   LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID)
{
   printf("�����������: %.0f %s �� %d ����\r", ceil(100 * (float)TotalBytesTransferred.LowPart / 
      (float)TotalFileSize.LowPart), "%", TotalFileSize.LowPart);
   return 0;
}

// ������ ���� � �������. ����������� "2014-12-10 00:00:00" ��� "2014-12-10" ��� "00:00:00"
struct tm ParseDateTime(char *s)
{
   struct tm dt;
   char buff[256];

   if (strlen(s) >= 10) // ����
   {
      strncpy_s(buff, sizeof(buff), s, 4);
      dt.tm_year = atoi(buff);
      strncpy_s(buff, sizeof(buff), s + 5, 2);
      dt.tm_mon = atoi(buff);
      strncpy_s(buff, sizeof(buff), s + 8, 2);
      dt.tm_mday = atoi(buff);

      if (strlen(s) == 19) // ���� � �����
      {
         strncpy_s(buff, sizeof(buff), s + 11, 2);
         dt.tm_hour = atoi(buff);
         strncpy_s(buff, sizeof(buff), s + 14, 2);
         dt.tm_min = atoi(buff);
         strncpy_s(buff, sizeof(buff), s + 17, 2);
         dt.tm_sec = atoi(buff);
      }
   }
   else if (strlen(s) == 8) // ������ �����
   {
      strncpy_s(buff, sizeof(buff), s, 2);
      dt.tm_hour = atoi(buff);
      strncpy_s(buff, sizeof(buff), s + 3, 2);
      dt.tm_min = atoi(buff);
      strncpy_s(buff, sizeof(buff), s + 6, 2);
      dt.tm_sec = atoi(buff);
   }

   return dt;
}

// ��������� ��������� dt2 >= dt1 ?
bool CompareDateTime(struct tm dt1, struct tm dt2)
{
   if (dt2.tm_year >= 0 && dt1.tm_year >= 0)
   {
      if (dt2.tm_year > dt1.tm_year)
      {
         return true;
      }
      else if (dt2.tm_year == dt1.tm_year)
      {
         if (dt2.tm_mon > dt1.tm_mon)
         {
            return true;
         }
         else if (dt2.tm_mon == dt1.tm_mon)
         {
            if (dt2.tm_mday >= dt1.tm_mday)
            {
               return true;
            }
         }
      }
   }

   if (dt2.tm_hour >= 0 && dt1.tm_hour >= 0)
   {
      if (dt2.tm_hour > dt1.tm_hour)
      {
         return true;
      }
      else if (dt2.tm_hour == dt1.tm_hour)
      {
         if (dt2.tm_min > dt1.tm_min)
         {
            return true;
         }
         else if (dt2.tm_min == dt1.tm_min)
         {
            if (dt2.tm_sec >= dt1.tm_sec)
            {
               return true;
            }
         }
      }
   }

   return false;
}

// ���������� ������������ ��������� ��������
// ���� ������ �� ������, �� �� ���������
// ���� ������ ���� � ����� ������ ������, �� ������ ��������� ������� � ����
// ���� ������ �������� ���� � �������, �� ������ ����
// ���� ������� ������ ����, �� ����� ������������
// ���� ������� ������ �����, �� ��������� ������ � ��� ����� ������� ���
bool isFilterSatisfy(int argc, char* argv[], struct tm dtRec)
{
   struct tm dtFrom;		   // ����-����� ������ ����������
   struct tm dtTo;			// ����-����� ��������� ����������

   if (argc == 2)
   {
      dtFrom = ParseDateTime(argv[1]);
      return CompareDateTime(dtFrom, dtRec);
   }
   else if (argc == 3)
   {
      dtFrom = ParseDateTime(argv[1]);
      dtTo = ParseDateTime(argv[2]);
      return CompareDateTime(dtFrom, dtRec) && CompareDateTime(dtRec, dtTo);
   }

   return true;
}

int main(int argc, char* argv[])
{
   setlocale(LC_ALL, "Russian");
   FILE *ifile;					// ���������� ����� ���� ���������
   FILE *ofile;					// ���������� ��������� �����
   FILE *mfile;					// ���������� ����� � �������� ��������� ����������� ������
   SCDBinfo scdbi;				// c�������� ��������� ���� ���������
   SCRECinfo screci;				// c�������� ��������� ��������� ������ ���������
   WAVEinfo wavei;				// ��������� ��������� ��������� �����
   struct tm datatime;			// ��������� ����-�����
   __time64_t loctime;			// ���-�� ��� ��������� � 0:00:00 1 ������ 1970 �� ������ ������
   char ifilename[256];			// ��� ����� ���� ���������
   char ofilename[256];			// ��� ��������� �����
   char mfilename[256];			// ��� ����� � �������� ��������� ����������� ������
   __int64 lastPos;				// �������� � ���� ��������� ����������� ������
   __int32 freqPrev = 0;		// �������� ������� ���������� ������
   __int32 datatimePrev = 0;	// ���� � ����� ����� ���������� ������
   __int32 lenghtTotal = 0;	// ��������� ����� �������� ������ � ������
   __int8 *dataTotal = 0;		// ����� ����������� ����������� (�������)
   __int8 *dataTotalBuff = 0;	// �������������� ����� ����������� ����������� (�������)
   char buff[256];

   printf("\n������������ ��� Signal Intelligence ScanStar Spectrum Commander IX\n");
   printf("����������� ������� �.�. (c) 13.12.2014 mailto:ivahnov@mail.ru\n");

   //char *arr[3];
   //arr[1] = new char[100];
   //arr[2] = new char[100];
   //strcpy(arr[1], "2014-11-10"); //from
   //strcpy(arr[2], "2014-11-10"); // to
   //bool res = isFilterSatisfy(3, arr, ParseDateTime("2014-12-05 17:25:38"));
   //exit(0);

   // �������� �������� � ��������� ������ ����� ��� ���������
   if (argc < 2)
   {
      printf("\n��� ������� ���������� ��������� � ��������� ������ ��� ���� ���������");
      printf("\n����������� �������� ��������� ���������� ���� � ����� � ��������:");
      printf("\n\"yyyy-MM-dd hh:mm:ss\" ��� \"yyyy-MM-dd\" ��� \"hh:mm:ss\"\n\n");
      printf("������� ����� ������� ��� ������...\n");
      _gettch();
      exit(0);
   }

   // ��� � ���� � ���� ���������
   strcpy_s(ifilename, sizeof(ifilename), argv[1]);
   strncpy_s(mfilename, sizeof(mfilename), argv[1], strlen(argv[1]) - 4);
   //strcpy_s(ifilename, sizeof(ifilename), "D:\\Banks\\PCR1000-1_RECORDED_AUDIO.SWQ");
   //strcpy_s(mfilename, sizeof(mfilename), "D:\\Banks\\PCR1000-1_RECORDED_AUDIO");
   strcat_s(mfilename, sizeof(mfilename), ".ind");

   // ������ ������� ��������� ����������� ������
   if (fopen_s(&mfile, mfilename, "rb") != 0)
   {
      lastPos = 288L;
   }
   else
   {
      fread(&lastPos, sizeof(lastPos), 1, mfile);
      fclose(mfile);
   }

   // ������� �������� ����� ����
   if (fopen_s(&ifile, ifilename, "rb") != 0)
   {
      // ���� ���� �� ������� �������, �� ��� ��� ��� �� ����� ����������
      // ������� ����� � ���� ��������� ����� � �������� � ���
      strncpy_s(buff, sizeof(buff), ifilename, strlen(ifilename) - 4);
      strcat_s(buff, sizeof(buff), ".tmp");
      printf("\n���� \"%s\" �� ���������� ��� ����� ����������\n", ifilename);
      printf("������ ��������� ����� ����� ����...\n");
      if (!CopyFileExA(ifilename, buff, CopyProgressRoutine, NULL, NULL, NULL))
      {
         printf("\n���� \"%s\" �� ����������, ��������� ������������ ���� � �����\n", ifilename);
         printf("������� ����� ������� ��� ������...\n");
         _gettch();
         exit(0);
      }
      else
      {
         printf("��������� ����� ����� ���� ������� �������\n");
         strcpy_s(ifilename, sizeof(ifilename), buff);
         fopen_s(&ifile, ifilename, "rb");
      }
   }

   // ������ ���������� � ����
   fread(&scdbi, sizeof(scdbi), 1, ifile);

   printf("\n���������� �������� ������� �� %s\n", ifilename);
   printf("���������� ������� � ����: %d\n", scdbi.count);
   printf("���������� � �������: %d\n", lastPos);
   // �������� ������� PCM, � ������� �� ��������
   if (scdbi.format == 1)
   {
      printf("������ �������: PCM\n");
   }
   else
   {
      printf("�� �������������� ������ ����� (%d)\n", scdbi.format);
      printf("������� ����� ������� ��� ������...\n");
      _gettch();
      fclose(ifile);
      if (strstr(ifilename, ".tmp"))
      {
         remove(ifilename);
      }
      exit(0);
   }
   printf("���������� �������: %d\n", scdbi.nchans);
   printf("������� �������������: %d\n", scdbi.srate);
   printf("�������� �������� �����: %d\n", scdbi.bitrate);
   printf("���������� ���� � ������: %d\n", scdbi.nbytes);
   printf("���������� ��� � ������: %d\n", scdbi.nbits);

   // �������� ���������� � ������ ���� ��������� � ������������ ��������� ���� � buff
   strncpy_s(buff, sizeof(buff), ifilename, (int)strrchr(ifilename, '\\') - (int)ifilename);
   _chdir(buff);
   strcpy_s(buff, sizeof(buff), (const char *)((int)strrchr(ifilename, '\\') + 1));
   strncpy_s(buff, sizeof(buff), buff, strlen(buff) - 4);
   _mkdir(buff);
   strncpy_s(ofilename, sizeof(ofilename), ifilename, strlen(ifilename) - 4);
   strcpy_s(buff, sizeof(buff), ofilename);

   wavei.triff = 1179011410;
   wavei.twave = 1163280727;
   wavei.tfmt = 544501094;
   wavei.n16 = 16;
   wavei.format = scdbi.format;
   wavei.nchans = scdbi.nchans;
   wavei.srate = scdbi.srate;
   wavei.bitrate = scdbi.bitrate;
   wavei.nbytes = scdbi.nbytes;
   wavei.nbits = scdbi.nbits;
   wavei.tdata = 1635017060;
   screci.index = 1;

   // ������� � ������ ���� ������� �� ���� ��������� � ���� �� ���� ���������
   _fseeki64(ifile, lastPos, SEEK_SET);
   while (screci.index <= scdbi.count + 1)
   {
      if (screci.index < scdbi.count)
      {
         fread(&screci, sizeof(screci), 1, ifile);
         loctime = screci.datatime;
         _localtime64_s(&datatime, &loctime);
      }
      else
      {
         screci.index++;
      }
      if (screci.adrCurr < 0)
      {
         _fseeki64(ifile, lastPos, SEEK_SET);
         break;
      }
      else
      {
         lastPos = screci.adrCurr;
      }
      // ���� ��� ����� ������ ��� ���������
      if ((screci.freq != freqPrev) || (screci.datatime > datatimePrev + 60) || (screci.index == scdbi.count + 1))
      {
         if (freqPrev && lenghtTotal && isFilterSatisfy(argc, argv, datatime)) // ��������� ����� ����� �������
         {
            fopen_s(&ofile, ofilename, "wb");
            fwrite(&wavei, sizeof(wavei), 1, ofile);
            fwrite(dataTotal, 1, lenghtTotal, ofile);
            fclose(ofile);
            printf("\n[%.5d/%.5d] %s", screci.index, scdbi.count, (const char *)((int)strrchr(ofilename, '\\') + 1));
         }
         delete [] dataTotal;
         dataTotal = 0;
         lenghtTotal = 0;
         if (screci.index == scdbi.count)
         {
            break;
         }
         // �������������� ����� ��������� �����
         sprintf_s(ofilename, sizeof(ofilename), "%s\\%d_%.2d_%.2d-%.2d%.2d%.2d_%.9d", buff,
            datatime.tm_year + 1900, datatime.tm_mon + 1, datatime.tm_mday,
            datatime.tm_hour, datatime.tm_min, datatime.tm_sec, screci.freq);
         // �������������� ������������ �� ������ � ����������� � ������
         for (size_t i=0; i<strlen(screci.info); i++)
         {
            if (screci.info[i] == '\\' || screci.info[i] == '/' || screci.info[i] == '|' ||
               screci.info[i] == '<' || screci.info[i] == '>' || screci.info[i] == '\"' ||
               screci.info[i] == '*' || screci.info[i] == ':' || screci.info[i] == '?')
            {
               screci.info[i] = '-';
            }
         }
         // ������ ���������� � ������ � ��� �����, ���� ��� �������
         if (strlen(screci.info))
         {
            sprintf_s(ofilename, sizeof(ofilename), "%s %s.wav", ofilename, screci.info);
         }
         else
         {
            sprintf_s(ofilename, sizeof(ofilename), "%s.wav", ofilename);
         }
      }

      lenghtTotal += screci.lenght;
      wavei.ndata = lenghtTotal;
      wavei.lenght = wavei.ndata + sizeof(WAVEinfo) - 8;

      // �������� ����������� ������� ������
      __int8 *data = new __int8 [screci.lenght];
      fread(data, 1, screci.lenght, ifile);

      if (dataTotal != 0)
      {
         // ��������� ������ �� dataTotal � dataTotalBuff
         dataTotalBuff = new __int8 [lenghtTotal-screci.lenght];
         for (int i=0; i<lenghtTotal-screci.lenght; i++)
         {
            dataTotalBuff[i] = dataTotal[i];
         }
         delete [] dataTotal;
      }
      // ����������� ������ dataTotal
      dataTotal = new __int8 [lenghtTotal];
      if (dataTotalBuff != 0)
      {
         // ��������������� ���������� dataTotal
         for (int i=0; i<lenghtTotal-screci.lenght; i++)
         {
            dataTotal[i] = dataTotalBuff[i];
         }
         delete [] dataTotalBuff;
         dataTotalBuff = 0;
      }
      // ��������� ����� ������ � dataTotal
      for (int i=lenghtTotal-screci.lenght; i<lenghtTotal; i++)
      {
         dataTotal[i] = data[i-lenghtTotal+screci.lenght];
      }
      delete [] data;

      freqPrev = screci.freq;
      datatimePrev = screci.datatime + (__int32)((float)screci.lenght/(float)scdbi.bitrate);
   }

   printf("\n\n��� ������ ��������� �������\n", scdbi.nbits);
   //_gettch();

   fclose(ifile);
   if (strstr(ifilename, ".tmp"))
   {
      remove(ifilename);
   }

   // ���������� ������� ��������� ����������� ������
   fopen_s(&mfile, mfilename, "wb");
   fwrite(&lastPos, 1, sizeof(lastPos), mfile);
   fclose(mfile);

   return 0;
}