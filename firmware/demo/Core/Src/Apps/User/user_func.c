#include "user_func.h"
#include "SD/sd_func.h"
#include "ap_oled.h"

/* 한 줄 명령 해석 및 실행 */
void userInput(char *command)
{
  printf("Command received: %s\r\n", command);

  if (strcmp(command, "help") == 0)
  {
    showHelp();
  }
  else if (strcmp(command, "mount") == 0)
  {
    sdCardMount();
  }
  else if (strcmp(command, "unmount") == 0)
  {
    sdCardUnMount();
  }
  else if (strncmp(command, "open ", 5) == 0)
  {
    openFile(command + 5);
  }
  else if (strcmp(command, "close") == 0)
  {
    closeFile();
  }
  else if (strncmp(command, "write ", 6) == 0)
  {
    writeFile(command + 6);
  }
  else if (strncmp(command, "read ", 5) == 0)
  {
    readFile(command + 5);
  }
  else if (strncmp(command, "image ", 6) == 0)
  {
    apOledDisplayImageFromSD(command + 6);
  }
  else if (strcmp(command, "size") == 0)
  {
    checkSize();
  }
  else
  {
    printf("Unknown command: %s\r\nType 'help' for available commands.\r\n", command);
  }
}

/* 명령 도움말 */
void showHelp(void)
{
  printf("\r\n=== Available Commands ===\r\n");
  printf("help               - Show this help\r\n");
  printf("mount              - Mount SD card\r\n");
  printf("unmount            - Unmount SD card\r\n");
  printf("open <filename>    - Open (create if not exist) & append mode\r\n");
  printf("close              - Close current file\r\n");
  printf("write <text>       - Append one line to current file\r\n");
  printf("read <filename>    - Read file content (up to buffer size)\r\n");
  printf("image <filename>   - Display 32bpp BMP on OLED\r\n");
  printf("size               - Show SD free/total space (KB)\r\n");
  printf("camera             - OLED camera mode (if available)\r\n");
  printf("ani <Num>          - OLED animation mode\r\n");
  printf("==========================\r\n");
}
