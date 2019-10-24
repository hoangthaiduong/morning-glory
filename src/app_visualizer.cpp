#define ENTRY_CONFIG_IMPLEMENT_MAIN 1
#define USE_ENTRY 1
#include <entry/entry.h>
#include <entry/entry.cpp>
#include <entry/entry_windows.cpp>
#include <entry/input.cpp>
#include <entry/cmd.cpp>
#include <imgui/imgui.h>
#include <common.h>
#include <bgfx_utils.h>
#include <stdio.h>
#include <math.h>
#include <bx/string.h>
#include <bx/timer.h>
#include <bimg/decode.h>
#include <nanovg/nanovg.h>
#include <nanovg/nanovg.cpp>
#include <nanovg/nanovg_bgfx.cpp>
#define BLENDISH_IMPLEMENTATION
#include <blendish.h>
#include <meshoptimizer/src/vertexcodec.cpp>
#include <meshoptimizer/src/indexcodec.cpp>
#include "mg_common.h"
#include "mg_math.h"
#include "mg_rendering.h"
#include "mg_volume.h"
#include "mg_wavelet.h"
#include "mg_all.cpp"
#define NOC_FILE_DIALOG_WIN32
#define NOC_FILE_DIALOG_IMPLEMENTATION
#include "noc_file_dialog.h"

namespace
{

using namespace mg;

#define ICON_SEARCH 0x1F50D
#define ICON_CIRCLED_CROSS 0x2716
#define ICON_CHEVRON_RIGHT 0xE75E
#define ICON_CHECK 0x2713
#define ICON_LOGIN 0xE740
#define ICON_TRASH 0xE729

// Returns 1 if col.rgba is 0.0f,0.0f,0.0f,0.0f, 0 otherwise
int isBlack( struct NVGcolor col )
{
	if( col.r == 0.0f && col.g == 0.0f && col.b == 0.0f && col.a == 0.0f )
	{
		return 1;
	}
	return 0;
}

static char* cpToUTF8(int cp, char* str)
{
	int n = 0;
	if (cp < 0x80) n = 1;
	else if (cp < 0x800) n = 2;
	else if (cp < 0x10000) n = 3;
	else if (cp < 0x200000) n = 4;
	else if (cp < 0x4000000) n = 5;
	else if (cp <= 0x7fffffff) n = 6;
	str[n] = '\0';
	switch (n)
	{
		case 6: str[5] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x4000000; BX_FALLTHROUGH;
		case 5: str[4] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x200000;  BX_FALLTHROUGH;
		case 4: str[3] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x10000;   BX_FALLTHROUGH;
		case 3: str[2] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x800;     BX_FALLTHROUGH;
		case 2: str[1] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0xc0;      BX_FALLTHROUGH;
		case 1: str[0] = char(cp);                                          BX_FALLTHROUGH;
	}
	return str;
}

void 
DrawSubbandSep(NVGcontext* Vg, const v2i& TopLeft, const v2i& N, const v2i& Spacing, int NLevels) {
	nvgSave(Vg);
	nvgStrokeColor(Vg, nvgRGBA(50,0,0,255));
  nvgStrokeWidth(Vg, 1);
  v2i M = N;
  for (int L = 0; L < NLevels; ++L) {
    M = (M + 1) / 2;
    nvgBeginPath(Vg);
    nvgMoveTo(Vg, TopLeft.X + Spacing.X * (M.X - 1) + Spacing.X / 2, TopLeft.Y - Spacing.X / 2);
    nvgLineTo(Vg, TopLeft.X + Spacing.X * (M.X - 1) + Spacing.X / 2, TopLeft.Y + Spacing.Y * (M.Y - 1) * 2 + Spacing.Y / 2);
    nvgStroke(Vg);
    nvgBeginPath(Vg);
    nvgMoveTo(Vg, TopLeft.X - Spacing.X / 2, TopLeft.Y + (M.Y - 1) * Spacing.Y + Spacing.Y / 2);
    nvgLineTo(Vg, TopLeft.X + Spacing.X * (M.X - 1) * 2 + Spacing.X / 2, TopLeft.Y + (M.Y - 1) * Spacing.Y + Spacing.Y / 2);
    nvgStroke(Vg);
  }
	nvgRestore(Vg);
}

void
DrawBlockSep(NVGcontext* Vg, const v2i& TopLeft, const v2i& N, const v2i& B, const v2i& Spacing) {
	nvgSave(Vg);
	nvgStrokeColor(Vg, nvgRGBA(50,0,0,255));
  nvgStrokeWidth(Vg, 1);
  v2i NBlocks = (N + B - 1) / B;
  for (int I = 1; I < NBlocks.X; ++I) {
    nvgBeginPath(Vg);
    nvgMoveTo(Vg, TopLeft.X + Spacing.X * (I * B.X - 1) + Spacing.X / 2, TopLeft.Y - Spacing.X / 2);
    nvgLineTo(Vg, TopLeft.X + Spacing.X * (I * B.X - 1) + Spacing.X / 2, TopLeft.Y + Spacing.Y * (N.Y - 1) + Spacing.Y / 2);
    nvgStroke(Vg);
  }
  for (int I = 0; I < NBlocks.Y; ++I) {
    nvgBeginPath(Vg);
    nvgMoveTo(Vg, TopLeft.X - Spacing.X / 2, TopLeft.Y + (I * B.Y - 1) * Spacing.Y + Spacing.Y / 2);
    nvgLineTo(Vg, TopLeft.X + Spacing.X * (N.X - 1) + Spacing.X / 2, TopLeft.Y + (I * B.Y - 1) * Spacing.Y + Spacing.Y / 2);
    nvgStroke(Vg);
  }
	nvgRestore(Vg);
}

enum class draw_mode { Fill, Stroke, FillStroke };
void 
DrawGrid(NVGcontext* Vg, const v2i& From, const v2i& Dims, const v2i& Spacing, 
  const NVGcolor& Color, draw_mode Mode, int StrokeWidth = 1) 
{
	nvgSave(Vg);
  nvgStrokeWidth(Vg, StrokeWidth);
  if (Mode != draw_mode::Stroke)
    nvgFillColor(Vg, Color);
  if (Mode != draw_mode::Fill)
    nvgStrokeColor(Vg, Color);
	for (int Y = From.Y; Y < From.Y + Dims.Y * Spacing.Y; Y += Spacing.Y) {
		for (int X = From.X; X < From.X + Dims.X * Spacing.X; X += Spacing.X) {
			nvgBeginPath(Vg);
			//nvgCircle(Vg, X, Y, 4);
      nvgRect(Vg, X - 4, Y - 4, 8, 8);
      //nvgStrokeColor(Vg, nvgRGBA(0,130,0,200) );
      if (Mode != draw_mode::Fill)
        nvgStroke(Vg);
      if (Mode != draw_mode::Stroke)
			  nvgFill(Vg);
		}
	}
	nvgRestore(Vg);
}

NVGcolor Int32ToColor(int Val) {
  v3i C = Unpack3i32(Val);
  return nvgRGBA(C.R, C.G, C.B, 255);
}

void DrawGrid(NVGcontext* Vg, const v2i& From, const v2i& N, const v2i& Spacing,
  const volume& VolColor, draw_mode Mode) 
{
	nvgSave(Vg);
  nvgStrokeWidth(Vg,1.0f);
  v2i D3 = Dims(VolColor).XY;
	for (int Y = From.Y; Y < From.Y + N.Y * Spacing.Y; Y += Spacing.Y) {
		for (int X = From.X; X < From.X + N.X * Spacing.X; X += Spacing.X) {
      int YY = (Y - From.Y) / Spacing.Y;
      int XX = (X - From.X) / Spacing.X;
      int C = VolColor.At<i32>(v3i(XX, YY, 0));
      NVGcolor Color = Int32ToColor(C);
      if (Mode != draw_mode::Stroke)
        nvgFillColor(Vg, Color);
      if (Mode != draw_mode::Fill)
        nvgStrokeColor(Vg, Color);
			nvgBeginPath(Vg);
      nvgRect(Vg, X - 4, Y - 4, 8, 8);
      if (Mode != draw_mode::Fill)
        nvgStroke(Vg);
      if (Mode != draw_mode::Stroke)
			  nvgFill(Vg);
		}
	}
	nvgRestore(Vg);
}

void DrawBox(NVGcontext* Vg, const v2i& From, const v2i& To, 
  const NVGcolor& Color, draw_mode Mode = draw_mode::Fill) 
{
	nvgSave(Vg);
	nvgBeginPath(Vg);
	nvgRect(Vg, From.X, From.Y, To.X - From.X, To.Y - From.Y);
  if (Mode != draw_mode::Stroke) {
	  nvgFillColor(Vg, Color);
    nvgFill(Vg);
  }
  if (Mode != draw_mode::Stroke) {
    nvgStrokeColor(Vg, Color);
    nvgFill(Vg);
  }
	nvgRestore(Vg);
}

void DrawText(NVGcontext* Vg, const v2i& Where, cstr Text, int Size) {
  nvgSave(Vg);
	nvgFillColor(Vg, nvgRGBA(58,30,34,250));
  nvgFontSize(Vg, Size);
  nvgFontFace(Vg, "sans");
  nvgTextAlign(Vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
	nvgFontBlur(Vg, 0);
  nvgText(Vg, Where.X, Where.Y, Text, nullptr);
  nvgRestore(Vg);
}

struct box {
  v2i From;
  v2i To;
};
box GetPointBox(const v2i& MouseDown, const v2i& MouseUp, const v2i& TopLeft,
  const v2i& N, const v2i& Spacing)
{
  v2i MouseFrom = Max(Min(MouseDown, MouseUp), TopLeft);
  v2i MouseTo = Min(Max(MouseDown, MouseUp), TopLeft + N * Spacing);
  int PointFromX = (int)ceil(float(MouseFrom.X - TopLeft.X) / Spacing.X);
  int PointFromY = (int)ceil(float(MouseFrom.Y - TopLeft.Y) / Spacing.Y);
  int PointToX = (int)floor(float(MouseTo.X - TopLeft.X) / Spacing.X);
  int PointToY = (int)floor(float(MouseTo.Y - TopLeft.Y) / Spacing.Y);
  return box{ v2i(PointFromX, PointFromY), v2i(PointToX, PointToY) };
}

void drawWindow(struct NVGcontext* vg, const char* title, float x, float y, float w, float h)
{
	float cornerRadius = 3.0f;
	struct NVGpaint shadowPaint;
	struct NVGpaint headerPaint;

	nvgSave(vg);
	//fonsClearState(vg);

	// Window
	//nvgBeginPath(vg);
	//nvgRect(vg, x,y, w,h);
	//nvgFillColor(vg, nvgRGBA(28,30,34,192) );
	//nvgFillColor(vg, nvgRGBA(0,0,0,128) );
	//nvgFill(vg);

	// Header
	//nvgBeginPath(vg);
	//nvgRoundedRect(vg, x+1,y+1, w-2,30, cornerRadius-1);
	//nvgFillColor(vg, nvgRGBA(240,30,34,192));
	//nvgFill(vg);
	//nvgBeginPath(vg);
	//nvgMoveTo(vg, x+0.5f, y+0.5f+30);
	//nvgLineTo(vg, x+0.5f+w-1, y+0.5f+30);
	//nvgStrokeColor(vg, nvgRGBA(0,0,0,32) );
	//nvgStroke(vg);

	nvgFontSize(vg, 18.0f);
	nvgFontFace(vg, "sans-bold");
	nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_BOTTOM);

	//nvgFontBlur(vg,2);
	//nvgFillColor(vg, nvgRGBA(0,0,0,128) );
	//nvgText(vg, x+w/2,y+16+1, title, NULL);

	nvgFontBlur(vg,0);
	nvgFillColor(vg, nvgRGBA(220,220,220,160) );
	nvgText(vg, x+w/2,y+16, title, NULL);

	nvgRestore(vg);
}


struct DemoData
{
	int fontNormal, fontBold, fontIcons, fontEmoji;
	int images[12];
};

int32_t createImage(struct NVGcontext* _ctx, const char* _filePath, int _imageFlags)
{
	uint32_t size;
	void* data = load(_filePath, &size);
	if (NULL == data)
	{
		return 0;
	}

	bimg::ImageContainer* imageContainer = bimg::imageParse(
		  entry::getAllocator()
		, data
		, size
		, bimg::TextureFormat::RGBA8
		);
	unload(data);

	if (NULL == imageContainer)
	{
		return 0;
	}

	int32_t texId = nvgCreateImageRGBA(
		  _ctx
		, imageContainer->m_width
		, imageContainer->m_height
		, _imageFlags
		, (const uint8_t*)imageContainer->m_data
		);

	bimg::imageFree(imageContainer);

	return texId;
}

int32_t createFont(NVGcontext* _ctx, const char* _name, const char* _filePath)
{
	uint32_t size;
	void* data = load(_filePath, &size);
	if (NULL == data)
	{
		return -1;
	}

	return nvgCreateFontMem(_ctx, _name, (uint8_t*)data, size, 0);
}

int loadDemoData(struct NVGcontext* vg, struct DemoData* data)
{
	int i;

	if (vg == NULL)
		return -1;

	for (i = 0; i < 12; i++)
	{
		char file[128];
		bx::snprintf(file, 128, "images/image%d.jpg", i+1);
		data->images[i] = createImage(vg, file, 0);
		if (data->images[i] == 0)
		{
			bx::debugPrintf("Could not load %s.\n", file);
			return -1;
		}
	}

	int32_t result = 0;

	data->fontIcons = createFont(vg, "icons", "font/entypo.ttf");
	if (data->fontIcons == -1)
	{
		bx::debugPrintf("Could not add font icons.\n");
		result = -1;
	}

	data->fontNormal = createFont(vg, "sans", "font/roboto-regular.ttf");
	if (data->fontNormal == -1)
	{
		bx::debugPrintf("Could not add font italic.\n");
		result = -1;
	}

	data->fontBold = createFont(vg, "sans-bold", "font/roboto-bold.ttf");
	if (data->fontBold == -1)
	{
		bx::debugPrintf("Could not add font bold.\n");
		result = -1;
	}

	data->fontEmoji = createFont(vg, "emoji", "font/NotoEmoji-Regular.ttf");
	if (data->fontEmoji == -1)
	{
		bx::debugPrintf("Could not add font emoji.\n");
		result = -1;
	}

	nvgAddFallbackFontId(vg, data->fontNormal, data->fontEmoji);
	nvgAddFallbackFontId(vg, data->fontBold, data->fontEmoji);

	return result;
}

void freeDemoData(struct NVGcontext* vg, struct DemoData* data)
{
	int i;

	if (vg == NULL)
		return;

	for (i = 0; i < 12; i++)
		nvgDeleteImage(vg, data->images[i]);
}


void renderDemo(struct NVGcontext* vg, float mx, float my, float width, float height, float t, int blowup, struct DemoData* data)
{
	float x,y,popx,popy;

	x = width-520; y = height-420;
	drawWindow(vg, "Widgets `n Stuff", x, y, 300, 400);
}

void ComputeWavelet(const volume& Vol, volume* Wav, int NLevels) {
  Clone(Vol, Wav);
  v3i N = Dims(Vol);
  for (int I = 0; I < NLevels; ++I) {
    FLiftCdf53OldX((f64*)Wav->Buffer.Data, N, v3i(I));
    FLiftCdf53OldY((f64*)Wav->Buffer.Data, N, v3i(I));
    FLiftCdf53OldZ((f64*)Wav->Buffer.Data, N, v3i(I));
  }
}

void ComputeVolColor(const volume& Vol, const transfer_func& Tf, volume* VolColor)
{
  v3i N = Dims(Vol);
  Resize(VolColor, N, dtype::int32);
  auto MM = MinMaxElem(Begin<f64>(Vol), End<f64>(Vol));
  f64 MinWav = *(MM.Min), MaxWav = *(MM.Max);
  for (int Y = 0; Y < N.Y; ++Y) {
    for (int X = 0; X < N.X; ++X) {
      f64 Val = (Vol.At<f64>(Y * N.X + X) - MinWav) / (MaxWav - MinWav);
      VolColor->At<u32>(v3i(Y, X, 0)) = Pack3i32(GetRGB(Tf, Val) * 255.0);
    }
  }
}

void ComputeWavColor(const volume& Vol, const array<extent>& Subbands,
  const transfer_func& Tf, volume* VolColor)
{
  v3i N = Dims(Vol);
  Resize(VolColor, N, dtype::int32);
  for (int I = 0; I < Size(Subbands); ++I) {
    v2i From2 = From(Subbands[I]).XY;
    v2i Dims2 = Dims(Subbands[I]).XY;
    auto MM = MinMaxElem(Begin<f64>(Subbands[I], Vol), End<f64>(Subbands[I], Vol));
    f64 MinWav = *(MM.Min), MaxWav = *(MM.Max);
    for (int Y = From2.Y; Y < From2.Y + Dims2.Y; ++Y) {
      for (int X = From2.X; X < From2.X + Dims2.X; ++X) {
        f64 Val = (Vol.At<f64>(Y * N.X + X) - MinWav) / (MaxWav - MinWav);
        VolColor->At<u32>(v3i(Y, X, 0)) = Pack3i32(GetRGB(Tf, Val) * 255.0);
      }
    }
  }
}

class ExampleNanoVG : public entry::AppI
{
public:
	ExampleNanoVG(const char* _name, const char* _description)
		: entry::AppI(_name, _description)
	{
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_NONE;
		m_reset  = BGFX_RESET_VSYNC;

		bgfx::Init init;
		init.type     = args.m_type;
		init.vendorId = args.m_pciId;
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		bgfx::init(init);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(0
			, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
			, 0xffffffff
			, 1.0f
			, 0
			);

		imguiCreate();

		m_nvg = nvgCreate(1, 0);
		bgfx::setViewMode(0, bgfx::ViewMode::Sequential);

		loadDemoData(m_nvg, &m_data);

		bndSetFont(nvgCreateFont(m_nvg, "droidsans", "font/droidsans.ttf") );
		bndSetIconImage(createImage(m_nvg, "images/blender_icons16.png", 0) );

		m_timeOffset = bx::getHPCounter();

    BuildSubbands(v3i(N, 1), NLevels, &Subbands);
    BuildSubbands(v3i(N, 1), NLevels, &SubbandsG);

    cstr TfFile = "D:/Datasets/ParaView Transfer Functions/rainbow-desaturated.json";
    cstr WavTfFile = "D:/Datasets/ParaView Transfer Functions/cool-warm-extended.json";
    //Dealloc(&Tf);
    auto Result = ReadTransferFunc(TfFile, &Tf);
    auto Result2 = ReadTransferFunc(WavTfFile, &WavTf);
    if (!Result || !Result2) {
      printf("Error: %s\n", ToString(Result));
    } else {
      cstr DataFile = "D:/Datasets/3D/Small/MIRANDA-PRESSURE-[33-33-1]-Float64.raw";
      //Dealloc(&Vol);
      Result = ReadVolume(DataFile, v3i(N, 1), dtype::float64, &Vol);
      if (!Result) {
        printf("Error: %s\n", ToString(Result));
      } else if (Size(Vol.Buffer) != Prod(N) * SizeOf(dtype::float64)) {
        printf("Error: size mismatch\n");
        Dealloc(&Vol);
      } else { // succeed
        ComputeVolColor(Vol, Tf, &VolColor);
        ComputeWavelet(Vol, &Wav, NLevels);
        ComputeWavColor(Wav, Subbands, WavTf, &WavColor);
      }
    }
	}

	int shutdown() override
	{
		freeDemoData(m_nvg, &m_data);

		nvgDelete(m_nvg);

		imguiDestroy();

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
		{
      const uint8_t* data = inputGetChar();
      const uint8_t character = data != nullptr ? 0u[data] : 0u;
			imguiBeginFrame(m_mouseState.m_mx
				,  m_mouseState.m_my
				, (m_mouseState.m_buttons[entry::MouseButton::Left  ] ? IMGUI_MBUT_LEFT   : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Right ] ? IMGUI_MBUT_RIGHT  : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				,  m_mouseState.m_mz
				, uint16_t(m_width)
				, uint16_t(m_height)
				, character);

			//showExampleDialog(this);
			static char buf1[8] = "33"; ImGui::InputText("Nx", buf1, 8, ImGuiInputTextFlags_CharsDecimal);
			static char buf2[8] = "33"; ImGui::InputText("Ny", buf2, 8, ImGuiInputTextFlags_CharsDecimal);
			if (ImGui::Button("Draw grid")) {
				ToInt(buf1, &N.X);
				ToInt(buf2, &N.Y);
			}
			if (!ImGui::GetIO().WantCaptureMouse) {
        int ValDomainBot = ValDomainTopLeft.Y + N.Y * Spacing.Y;
				if (m_mouseReleased && m_mouseState.m_buttons[entry::MouseButton::Left]) {
          if (m_mouseState.m_my < WavDomainTopLeft.Y - (WavDomainTopLeft.Y - ValDomainBot) / 2)
            ValMouseDown = v2i(m_mouseState.m_mx, m_mouseState.m_my);
          else
            WavMouseDown = v2i(m_mouseState.m_mx, m_mouseState.m_my);
					m_mouseReleased = false;
				}
				if (m_mouseState.m_buttons[entry::MouseButton::Left]) {
          if (m_mouseState.m_my < WavDomainTopLeft.Y - (WavDomainTopLeft.Y - ValDomainBot) / 2)
					  ValMouseUp = v2i(m_mouseState.m_mx, m_mouseState.m_my);
          else
					  WavMouseUp = v2i(m_mouseState.m_mx, m_mouseState.m_my);
				} else {
					m_mouseReleased = true;
				}
			}
			if (ImGui::SliderInt("Number of levels", &NLevels, 1, 4)) {
        BuildSubbands(v3i(N, 1), NLevels, &Subbands);
        BuildSubbands(v3i(N, 1), NLevels, &SubbandsG);
        ComputeWavelet(Vol, &Wav, NLevels);
        ComputeWavColor(Wav, Subbands, WavTf, &WavColor);
      }

			int64_t now = bx::getHPCounter();
			const double freq = double(bx::getHPFrequency() );
			float time = (float)( (now-m_timeOffset)/freq);

			// Set view 0 default viewport.
			bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height) );

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			bgfx::touch(0);

			nvgBeginFrame(m_nvg, m_width, m_height, 1.0f);

      // draw the val grid
			DrawGrid(m_nvg, ValDomainTopLeft, N, Spacing, VolColor, draw_mode::Fill);
			DrawGrid(m_nvg, WavDomainTopLeft, N, Spacing, WavColor, draw_mode::Fill);
      DrawSubbandSep(m_nvg, WavDomainTopLeft, N, Spacing, NLevels);
      // TODO: select the wavelet grid
      // TODO: draw the wavgrid, wrkgrid, valgrid
      // TODO: select between wrkgrid and valgrid with a radio button
      /* print the selection in the Val domain */
      box ValBox = GetPointBox(ValMouseDown, ValMouseUp, ValDomainTopLeft, N, Spacing);
      char Temp[50];
      sprintf(Temp, "(%d %d) to (%d %d)", ValBox.From.X, ValBox.From.Y, ValBox.To.X, ValBox.To.Y);
      //DrawText(m_nvg, v2i(50, 30), Temp, 20);
      //DrawBox(m_nvg, ValDomainTopLeft + ValBox.From * Spacing - Spacing / 2, ValDomainTopLeft + ValBox.From * Spacing + (ValBox.To - ValBox.From) * Spacing + Spacing / 2, nvgRGBA(230, 0, 0, 50));

      /* Wav domain */
      box WavBox = GetPointBox(WavMouseDown, WavMouseUp, WavDomainTopLeft, N, Spacing);
      //sprintf(Temp, "(%d %d) to (%d %d)", WavMouseDown.X, WavMouseDown.Y, WavMouseUp.X, WavMouseUp.Y);
      //DrawText(m_nvg, v2i(50, 530), Temp, 20);
      int WavFromX = (int)ceil(float(Max(0, Min(WavMouseDown.X, WavDomainTopLeft.X + N.X * Spacing.X)) - WavDomainTopLeft.X) / Spacing.X);
      int WavFromY = (int)ceil(float(Max(0, Min(WavMouseDown.Y, WavDomainTopLeft.Y + N.Y * Spacing.Y)) - WavDomainTopLeft.Y) / Spacing.Y);
      int Sb = -1;
      for (int I = 0; I < Size(Subbands); ++I) {
        if (v3i(WavFromX, WavFromY, 0) >= From(Subbands[I]) && v3i(WavFromX, WavFromY, 0) <= To(Subbands[I])) {
          Sb = I;
          break;
        }
      }
      extent WavCrop = Crop(extent(v3i(WavBox.From, 0), v3i(WavBox.To - WavBox.From + 1, 1)), Subbands[Sb]);
      DrawGrid(m_nvg, WavDomainTopLeft + From(WavCrop).XY * Spacing, (To(WavCrop) - From(WavCrop)).XY, Spacing, nvgRGBA(30, 0, 0, 200), draw_mode::Stroke, 2);
      sprintf(Temp, "(%d %d) to (%d %d)", From(WavCrop).X, From(WavCrop).Y, To(WavCrop).X, To(WavCrop).Y);
      //DrawText(m_nvg, v2i(50, 450), Temp, 20);
      sprintf(Temp, "Subband %d", Sb);
      //DrawText(m_nvg, v2i(50, 470), Temp, 20);

      /* WavGrid domain */
			DrawGrid(m_nvg, WavGDomainTopLeft, N, Spacing, nvgRGBA(0, 130, 0, 100), draw_mode::Stroke);
      extent WavCropLocal = WavCrop;
      SetFrom(&WavCropLocal, From(WavCrop) - From(Subbands[Sb]));
      grid WavGGrid = SubGrid(SubbandsG[Sb], WavCropLocal);
      sprintf(Temp, "from (%d %d) dims (%d %d) stride (%d %d)", From(WavGGrid).X, From(WavGGrid).Y, Dims(WavGGrid).X, Dims(WavGGrid).Y, Strd(WavGGrid).X, Strd(WavGGrid).Y);
      //DrawText(m_nvg, v2i(500, 30), Temp, 20);
			DrawGrid(m_nvg, WavGDomainTopLeft + From(WavGGrid).XY * Spacing, Dims(WavGGrid).XY, Spacing * Strd(WavGGrid).XY, nvgRGBA(0, 130, 0, 200), draw_mode::Stroke);
      //DrawBox(m_nvg, WavGDomainTopLeft + ValBox.From * Spacing - Spacing / 2, WavGDomainTopLeft + ValBox.From * Spacing + (ValBox.To - ValBox.From) * Spacing + Spacing / 2, nvgRGBA(230, 0, 0, 50));
      extent Footprint = WavFootprint(2, Sb, WavGGrid);
      //DrawBox(m_nvg, WavGDomainTopLeft + From(Footprint).XY * Spacing - Spacing / 2, WavGDomainTopLeft + From(Footprint).XY * Spacing + (Dims(Footprint) - 1).XY * Spacing + Spacing / 2, nvgRGBA(0, 0, 230, 50));
      //extent ValExt(v3i(ValBox.From, 0), v3i(ValBox.To - ValBox.From + 1, 1));
      extent ValExt(v3i(N, 1));
      wav_grids WavGrids = ComputeWavGrids(2, Sb, ValExt, WavGGrid, v3i(1000));
      //DrawText(m_nvg, v2i(500, 50), Temp, 20);
      ImGui::Checkbox("WavGrid/WrkGrid", &DrawWavGrid);
      // draw wavgrid/wrkgrid
      if (DrawWavGrid)
			  DrawGrid(m_nvg, WavGDomainTopLeft + From(WavGrids.WavGrid).XY * Spacing, Dims(WavGrids.WavGrid).XY, Spacing * Strd(WavGrids.WavGrid).XY, nvgRGBA(130, 0, 130, 200), draw_mode::Stroke);
      else // wrkgrid
			  DrawGrid(m_nvg, WavGDomainTopLeft + From(WavGrids.WrkGrid).XY * Spacing, Dims(WavGrids.WrkGrid).XY, Spacing * Strd(WavGrids.WrkGrid).XY, nvgRGBA(230, 0, 0, 255), draw_mode::Stroke, 2);
      // draw the val grid
      //DrawGrid(m_nvg, ValDomainTopLeft + From(WavGrids.ValGrid).XY * Spacing, Dims(WavGrids.ValGrid).XY, Spacing * Strd(WavGrids.ValGrid).XY, nvgRGBA(130, 0, 0, 200), draw_mode::Stroke);
      // draw block separations
      DrawBlockSep(m_nvg, ValDomainTopLeft, N, v2i(BlockSize), Spacing);
      DrawBlockSep(m_nvg, WavGDomainTopLeft, N, v2i(BlockSize), Spacing);
      grid Rel; 
      if (ImGui::Button("Copy wavelets")) {
        // copy the wavelet coefficients over to the workgrid
        Resize(&WrkGrid, Dims(WavGrids.WrkGrid), dtype::float64);
        ZeroBuf(&WrkGrid.Buffer);
        Rel = Relative(WavGrids.WavGrid, WavGrids.WrkGrid);
        Copy(WavCrop, Wav, Rel, &WrkGrid);
        Resize(&WrkGridColor, Dims(WrkGrid), dtype::int32);
        Fill(Begin<i32>(WrkGridColor), End<i32>(WrkGridColor), Pack3i32(v3i(50, 50, 50)));
        Copy(WavCrop, WavColor, Rel, &WrkGridColor);
        /* (optionally) copy the data from the affected blocks */
        WavFp = WavFootprint(2, Sb, WavGrids.WavGrid);
        Clear(&AffectedBlocks);
        printf("wavfp from (%d %d) (%d %d) (%d %d)\n", From(WavFp).X, From(WavFp).Y, Dims(WavFp).X, Dims(WavFp).Y, Strd(WavFp).X, Strd(WavFp).Y);
        BegBlock = (From(WavFp)  / BlockSize).XY;
        EndBlock = ((Last(WavFp) ) / BlockSize).XY;
        printf("begin block %d %d end block %d %d", BegBlock.X, BegBlock.Y, EndBlock.X, EndBlock.Y);
      }
      /* render the work grid */
      if (WrkGrid.Buffer && WrkGridColor.Buffer) {
        DrawGrid(m_nvg, WavGDomainTopLeft + From(WavGrids.WrkGrid).XY * Spacing, Dims(WrkGridColor).XY, Spacing * Strd(WavGrids.WrkGrid).XY, WrkGridColor, draw_mode::Fill);
        DrawBox(m_nvg, WavGDomainTopLeft + BegBlock * BlockSize * Spacing - Spacing / 2, WavGDomainTopLeft + (EndBlock + 1) * BlockSize * Spacing - Spacing / 2, nvgRGBA(0, 230, 0, 20));
        //DrawBox(m_nvg, WavGDomainTopLeft + From(WavFp).XY * Spacing - Spacing / 2, WavGDomainTopLeft + Last(WavFp).XY * Spacing + Spacing / 2, nvgRGBA(230, 0, 0, 50));
      }

			imguiEndFrame();
			nvgEndFrame(m_nvg);

			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();

			return true;
		}

		return false;
	}

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;

	entry::MouseState m_mouseState;
	v2i ValMouseDown = v2i::Zero;
	v2i ValMouseUp = v2i::Zero;
	v2i WavMouseDown = v2i::Zero;
	v2i WavMouseUp = v2i::Zero;
  volume WrkGrid;
  volume WrkGridColor;
	bool m_mouseReleased = true;
  bool DrawWavGrid = true;
  transfer_func Tf;
  transfer_func WavTf;
  volume Vol;
  volume Wav;
  f64 MinVal, MaxVal;
  f64 MinWav, MaxWav;
  volume VolColor;
  volume WavColor;
  int BlockSize = 8;
  v2i BegBlock = v2i::Zero;
  v2i EndBlock = v2i::Zero;
  extent WavFp;

	int64_t m_timeOffset;

	NVGcontext* m_nvg;
	DemoData m_data;
  v2i ValDomainTopLeft = v2i(50, 50);
  v2i WavDomainTopLeft = v2i(50, 500);
  v2i WavGDomainTopLeft = v2i(500, 50);
	v2i N = v2i(33, 33); // total dimensions
  v2i Spacing = v2i(12, 12);
	int NLevels = 2;
  array<extent> Subbands;
  array<grid> SubbandsG;
  array<v2i> AffectedBlocks;
};

} // namespace

ENTRY_IMPLEMENT_MAIN(ExampleNanoVG, "20-nanovg", "NanoVG is small antialiased vector graphics rendering library.");
