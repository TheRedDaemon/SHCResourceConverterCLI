#pragma once

// CLI COMMANDS:

namespace COMMAND
{
  inline const char* TEST{ "test" };
  inline const char* EXTRACT{ "extract" };
  inline const char* PACK{ "pack" };
  inline const char* HELP{ "help" };
}

// CLI OPTIONS:

namespace OPTION
{
  inline const char* LOG{ "log" };
  inline const char* TGX_CODER_TRANSPARENT_PIXEL_TGX_COLOR{ "tgx-coder-transparent-pixel-tgx-color" };
  inline const char* TGX_CODER_TRANSPARENT_PIXEL_RAW_COLOR{ "tgx-coder-transparent-pixel-raw-color" };
  inline const char* TGX_CODER_PIXEL_REPEAT_THRESHOLD{ "tgx-coder-pixel-repeat-threshold" };
  inline const char* TGX_CODER_PADDING_ALIGNMENT{ "tgx-coder-padding-alignment" };
}
