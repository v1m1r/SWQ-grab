#include "stdafx.h"

typedef VOID (*MYPROC)(LPTSTR);

// Для корректной работы структур используется выравнивание по 1 байт!
// Структура базы сканстара снята методом Тыка в 16-ричном редакторе WinHex
// Структура заголовка базы сканстара
struct SCDBinfo
{
   __int32 par1;		// 535353
   __int32 par2;		// 1
   __int32 par3;		// похоже на размер
   __int32 par4;		// 256
   __int32 par5;		// похоже на абсолютное положение второй записи в базе
   __int32 count;		// количество записей в базе
   __int64 par7;		// нули 
   __int16 format;	// формат записей (WAVE_FORMAT_PCM 0x0001)
   __int16 nchans;	// кол-во каналов (1)
   __int32 srate;		// частота дискретизации (12000)
   __int32 bitrate;	// скорость байт в сек (24000)
   __int16 nbytes;	// кол-во байт в сэмпле (1)
   __int16 nbits;		// кол-во бит в сэмпле (8)
};

// Структура заголовка отдельной записи сканстара
struct SCRECinfo
{
   __int32 adrCurr;	// абсолютное положение текущей записи в базе
   __int32 adrPrev;	// абсолютное положение предыдущей записи в базе
   __int32 lenght;	// длина звуковой записи в байтах
   __int32 par4;		// 1
   __int8 par5[10];	// нули
   __int32 index;		// индекс записи в базе
   __int32 par7;		// 2
   __int32 freq;		// значение частоты (MMMKKKHHH)
   __int32 datatime;	// UTC - кол-во сек прошедших с 0:00:00 1 января 1970
   __int8 par10[18];	// нули
   __int8 par11[3];	// н/у числа
   char radio[20];	// имя приёмника
   char info[166];	// информация о канале
};

// Структура заголовка звукового файла формата WAVE
struct WAVEinfo
{
   __int32 triff;		// текст "RIFF" (1179011410)
   __int32 lenght;	// размер файла - 8 байт
   __int32 twave;		// текст "WAVE" (1163280727)
   __int32 tfmt;		// текст "fmt " (544501094)
   __int32 n16;		// 16
   __int16 format;	// формат (WAVE_FORMAT_PCM 0x0001)
   __int16 nchans;	// кол-во каналов (1)
   __int32 srate;		// частота дискретизации (12000)
   __int32 bitrate;	// скорость байт в сек (24000)
   __int16 nbytes;	// кол-во байт в сэмпле (1)
   __int16 nbits;		// кол-во бит в сэмпле (8)
   __int32 tdata;		// текст "data" (1635017060)
   __int32 ndata;		// кол-во байт в области данных
};

DWORD CALLBACK CopyProgressRoutine(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred,
                                   LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID)
{
   printf("Скопировано: %.0f %s из %d байт\r", ceil(100 * (float)TotalBytesTransferred.LowPart / 
      (float)TotalFileSize.LowPart), "%", TotalFileSize.LowPart);
   return 0;
}

// Парсер даты и времени. Допускается "2014-12-10 00:00:00" или "2014-12-10" или "00:00:00"
struct tm ParseDateTime(char *s)
{
   struct tm dt;
   char buff[256];

   if (strlen(s) >= 10) // дата
   {
      strncpy_s(buff, sizeof(buff), s, 4);
      dt.tm_year = atoi(buff);
      strncpy_s(buff, sizeof(buff), s + 5, 2);
      dt.tm_mon = atoi(buff);
      strncpy_s(buff, sizeof(buff), s + 8, 2);
      dt.tm_mday = atoi(buff);

      if (strlen(s) == 19) // дата и время
      {
         strncpy_s(buff, sizeof(buff), s + 11, 2);
         dt.tm_hour = atoi(buff);
         strncpy_s(buff, sizeof(buff), s + 14, 2);
         dt.tm_min = atoi(buff);
         strncpy_s(buff, sizeof(buff), s + 17, 2);
         dt.tm_sec = atoi(buff);
      }
   }
   else if (strlen(s) == 8) // только время
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

// Вычисляет выражение dt2 >= dt1 ?
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

// Возвращает соответствие критериям фильтров
// Если ничего не задано, то не фильтрует
// Если задано дата и время только начала, то записи выводятся начиная с него
// Если указан диапазон даты и времени, то внутри него
// Если указана только дата, то время игнорируется
// Если указано только время, то выводятся записи в это время каждого дня
bool isFilterSatisfy(int argc, char* argv[], struct tm dtRec)
{
   struct tm dtFrom;		   // дата-время начала фильтрации
   struct tm dtTo;			// дата-время окончания фильтрации

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
   FILE *ifile;					// дескриптор файла базы сканстара
   FILE *ofile;					// дескриптор звукового файла
   FILE *mfile;					// дескриптор файла с индексом последней извлечённой записи
   SCDBinfo scdbi;				// cтруктура заголовка базы сканстара
   SCRECinfo screci;				// cтруктура заголовка отдельной записи сканстара
   WAVEinfo wavei;				// структура заголовка звукового файла
   struct tm datatime;			// структура дата-время
   __time64_t loctime;			// кол-во сек прошедших с 0:00:00 1 января 1970 до начала записи
   char ifilename[256];			// имя файла базы сканстара
   char ofilename[256];			// имя звукового файла
   char mfilename[256];			// имя файла с индексом последней извлечённой записи
   __int64 lastPos;				// смещение в базе последней извлечённой записи
   __int32 freqPrev = 0;		// значение частоты предыдущей записи
   __int32 datatimePrev = 0;	// дата и время конца предыдущей записи
   __int32 lenghtTotal = 0;	// суммарная длина звуковой записи в байтах
   __int8 *dataTotal = 0;		// буфер склеиваемых аудиоданных (выборка)
   __int8 *dataTotalBuff = 0;	// дополнительный буфер склеиваемых аудиоданных (выборка)
   char buff[256];

   printf("\nАудиограббер баз Signal Intelligence ScanStar Spectrum Commander IX\n");
   printf("Программист Ивахнов В.А. (c) 13.12.2014 mailto:ivahnov@mail.ru\n");

   //char *arr[3];
   //arr[1] = new char[100];
   //arr[2] = new char[100];
   //strcpy(arr[1], "2014-11-10"); //from
   //strcpy(arr[2], "2014-11-10"); // to
   //bool res = isFilterSatisfy(3, arr, ParseDateTime("2014-12-05 17:25:38"));
   //exit(0);

   // Проверка указания в командной строке файла для обработки
   if (argc < 2)
   {
      printf("\nПри запуске необходимо указывать в командной строке имя базы сканстара");
      printf("\nОпционально задаются параметры фильтрации дата и время в форматах:");
      printf("\n\"yyyy-MM-dd hh:mm:ss\" или \"yyyy-MM-dd\" или \"hh:mm:ss\"\n\n");
      printf("Нажмите любую клавишу для выхода...\n");
      _gettch();
      exit(0);
   }

   // Имя с путём к базе сканстара
   strcpy_s(ifilename, sizeof(ifilename), argv[1]);
   strncpy_s(mfilename, sizeof(mfilename), argv[1], strlen(argv[1]) - 4);
   //strcpy_s(ifilename, sizeof(ifilename), "D:\\Banks\\PCR1000-1_RECORDED_AUDIO.SWQ");
   //strcpy_s(mfilename, sizeof(mfilename), "D:\\Banks\\PCR1000-1_RECORDED_AUDIO");
   strcat_s(mfilename, sizeof(mfilename), ".ind");

   // Чтение индекса последней извлечённой записи
   if (fopen_s(&mfile, mfilename, "rb") != 0)
   {
      lastPos = 288L;
   }
   else
   {
      fread(&lastPos, sizeof(lastPos), 1, mfile);
      fclose(mfile);
   }

   // Попытка открытия файла базы
   if (fopen_s(&ifile, ifilename, "rb") != 0)
   {
      // Если файл не удалось открыть, то его нет или он занят сканстаром
      // Пробуем снять с него временную копию и работать с ней
      strncpy_s(buff, sizeof(buff), ifilename, strlen(ifilename) - 4);
      strcat_s(buff, sizeof(buff), ".tmp");
      printf("\nФайл \"%s\" не существует или занят сканстаром\n", ifilename);
      printf("Создаём временную копию файла базы...\n");
      if (!CopyFileExA(ifilename, buff, CopyProgressRoutine, NULL, NULL, NULL))
      {
         printf("\nФайл \"%s\" не существует, проверьте правильность пути и имени\n", ifilename);
         printf("Нажмите любую клавишу для выхода...\n");
         _gettch();
         exit(0);
      }
      else
      {
         printf("Временная копия файла базы создана успешно\n");
         strcpy_s(ifilename, sizeof(ifilename), buff);
         fopen_s(&ifile, ifilename, "rb");
      }
   }

   // Чтение информации о базе
   fread(&scdbi, sizeof(scdbi), 1, ifile);

   printf("\nИзвлечение звуковых записей из %s\n", ifilename);
   printf("Количество записей в базе: %d\n", scdbi.count);
   printf("Извлечение с позиции: %d\n", lastPos);
   // Проверка формата PCM, с другими не работаем
   if (scdbi.format == 1)
   {
      printf("Формат записей: PCM\n");
   }
   else
   {
      printf("Не поддерживаемый формат аудио (%d)\n", scdbi.format);
      printf("Нажмите любую клавишу для выхода...\n");
      _gettch();
      fclose(ifile);
      if (strstr(ifilename, ".tmp"))
      {
         remove(ifilename);
      }
      exit(0);
   }
   printf("Количество каналов: %d\n", scdbi.nchans);
   printf("Частота дискретизации: %d\n", scdbi.srate);
   printf("Скорость передачи битов: %d\n", scdbi.bitrate);
   printf("Количество байт в сэмпле: %d\n", scdbi.nbytes);
   printf("Количество бит в сэмпле: %d\n", scdbi.nbits);

   // Создание директории с именем базы сканстара и формирование выходного пути в buff
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

   // Переход к записи базы которая не была извлечена и цикл по всем остальным
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
      // Если это новая запись или последняя
      if ((screci.freq != freqPrev) || (screci.datatime > datatimePrev + 60) || (screci.index == scdbi.count + 1))
      {
         if (freqPrev && lenghtTotal && isFilterSatisfy(argc, argv, datatime)) // сохраняем файлы кроме первого
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
         // Форматирование имени выходного файла
         sprintf_s(ofilename, sizeof(ofilename), "%s\\%d_%.2d_%.2d-%.2d%.2d%.2d_%.9d", buff,
            datatime.tm_year + 1900, datatime.tm_mon + 1, datatime.tm_mday,
            datatime.tm_hour, datatime.tm_min, datatime.tm_sec, screci.freq);
         // Преобразование спецсимволов из строки с информацией о канале
         for (size_t i=0; i<strlen(screci.info); i++)
         {
            if (screci.info[i] == '\\' || screci.info[i] == '/' || screci.info[i] == '|' ||
               screci.info[i] == '<' || screci.info[i] == '>' || screci.info[i] == '\"' ||
               screci.info[i] == '*' || screci.info[i] == ':' || screci.info[i] == '?')
            {
               screci.info[i] = '-';
            }
         }
         // Запись информации о канале в имя файла, если она имеется
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

      // Загрузка аудиоданных текущей записи
      __int8 *data = new __int8 [screci.lenght];
      fread(data, 1, screci.lenght, ifile);

      if (dataTotal != 0)
      {
         // Сохраняем данные из dataTotal в dataTotalBuff
         dataTotalBuff = new __int8 [lenghtTotal-screci.lenght];
         for (int i=0; i<lenghtTotal-screci.lenght; i++)
         {
            dataTotalBuff[i] = dataTotal[i];
         }
         delete [] dataTotal;
      }
      // Увеличиваем размер dataTotal
      dataTotal = new __int8 [lenghtTotal];
      if (dataTotalBuff != 0)
      {
         // Восстанавливаем содержимое dataTotal
         for (int i=0; i<lenghtTotal-screci.lenght; i++)
         {
            dataTotal[i] = dataTotalBuff[i];
         }
         delete [] dataTotalBuff;
         dataTotalBuff = 0;
      }
      // Добавляем новые данные в dataTotal
      for (int i=lenghtTotal-screci.lenght; i<lenghtTotal; i++)
      {
         dataTotal[i] = data[i-lenghtTotal+screci.lenght];
      }
      delete [] data;

      freqPrev = screci.freq;
      datatimePrev = screci.datatime + (__int32)((float)screci.lenght/(float)scdbi.bitrate);
   }

   printf("\n\nВсе записи извлечены успешно\n", scdbi.nbits);
   //_gettch();

   fclose(ifile);
   if (strstr(ifilename, ".tmp"))
   {
      remove(ifilename);
   }

   // Сохранение индекса последней извлечённой записи
   fopen_s(&mfile, mfilename, "wb");
   fwrite(&lastPos, 1, sizeof(lastPos), mfile);
   fclose(mfile);

   return 0;
}