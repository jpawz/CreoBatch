/**
Creo batch tool.
Actions:
1. Convert drw to pdf
2. Convert drw to dxf
3. Convert prt/asm to stp
 */

#include <ProCore.h>
#include <ProMenuBar.h>
#include <ProMessage.h>
#include <ProUICmd.h>
#include <ProUIDialog.h>
#include <ProUIPushbutton.h>
#include <ProUITextarea.h>
#include <ProUtil.h>
#include <ProWindows.h>
#include <ProWstring.h>
#include <ProPDF.h>
#include <vector>

using namespace std;

static uiCmdAccessState AccessAvailable(uiCmdAccessMode);
void printMessage(string);
void makeDialogWindow();
char* strToCharArr(string);
void initializeMsgFile();
void parseTextArea();

void cancelAction(char*, char*, ProAppData);
void exportToPdfAction(char*, char*, ProAppData);
void exportToDxfAction(char*, char*, ProAppData);
void exportToStpAction(char*, char*, ProAppData);
void export2dDrawing(ProImportExportFile, wstring);
void textAction(char*, char*, ProAppData);

ProFileName msgFile;
char dialogName[] = { "creobatch" };
char separator[] = { "Separator" };
char cancel[] = { "Cancel" };
char exportToPdf[] = { "Eksport do PDF" };
char exportToDxf[] = { "Eksport do DXF" };
char exportToStp[] = { "Eksport do STP" };
char textArea[] = { "TextArea" };

vector<wchar_t*> numbers;
const int textAreaLength = 2048;
static wchar_t textAreaValue[textAreaLength];

extern "C" int main(int argc, char** argv)
{
	ProToolkitMain(argc, argv);
	return 0;
}

extern "C" int user_initialize()
{
	ProErr err = PRO_TK_NO_ERROR;

	initializeMsgFile();

	uiCmdCmdId creo_batch_cmd_id;

	char actionName[] = "CreoBatch";
	ProCmdActionAdd(actionName, (uiCmdCmdActFn)makeDialogWindow,
		uiCmdPrioDefault, AccessAvailable, PRO_B_FALSE,
		PRO_B_FALSE, &creo_batch_cmd_id);
	char parentMenu[] = "Utilities";
	char buttonName[] = "Creo Batch";
	char buttonLabel[] = "Creo Batch";
	ProMenubarmenuPushbuttonAdd(parentMenu, buttonName, buttonName, buttonLabel, NULL,
		PRO_B_TRUE, creo_batch_cmd_id, msgFile);

	return 0;
}

extern "C" void user_terminate()
{
	return;
}

void makeDialogWindow()
{
	int exit_status;
	ProUIDialogCreate(dialogName, NULL);
	ProUIGridopts gridOpts;
	PRO_UI_GRIDOPTS_DEFAULT(gridOpts);

	ProWstring* label;
	ProArrayAlloc(1, sizeof(wchar_t), 1, (ProArray*)&label);
	label[0] = (wchar_t*)calloc(PRO_COMMENT_SIZE, sizeof(wchar_t));

	gridOpts.row = 1;
	gridOpts.column = 1;
	gridOpts.horz_cells = 2;
	gridOpts.vert_cells = 10;
	ProUIDialogTextareaAdd(dialogName, textArea, &gridOpts);
	ProUITextareaMaxlenSet(dialogName, textArea, textAreaLength);
	ProUITextareaActivateActionSet(dialogName, textArea, textAction, NULL);

	gridOpts.horz_cells = 1;
	gridOpts.vert_cells = 1;
	gridOpts.row = 1;
	gridOpts.column = 3;
	gridOpts.horz_resize = PRO_B_FALSE;
	gridOpts.vert_resize = PRO_B_FALSE;
	ProUIDialogPushbuttonAdd(dialogName, cancel, &gridOpts);
	ProStringToWstring(label[0], cancel);
	ProUIPushbuttonTextSet(dialogName, cancel, label[0]);
	ProUIPushbuttonActivateActionSet(dialogName, cancel, cancelAction, NULL);

	gridOpts.row = 2;
	ProUIDialogSeparatorAdd(dialogName, separator, &gridOpts);

	gridOpts.row = 3;
	ProUIDialogPushbuttonAdd(dialogName, exportToPdf, &gridOpts);
	ProStringToWstring(label[0], exportToPdf);
	ProUIPushbuttonTextSet(dialogName, exportToPdf, label[0]);
	ProUIPushbuttonActivateActionSet(dialogName, exportToPdf, exportToPdfAction, NULL);

	gridOpts.row = 4;
	ProUIDialogPushbuttonAdd(dialogName, exportToDxf, &gridOpts);
	ProStringToWstring(label[0], exportToDxf);
	ProUIPushbuttonTextSet(dialogName, exportToDxf, label[0]);
	ProUIPushbuttonActivateActionSet(dialogName, exportToDxf, exportToDxfAction, NULL);

	gridOpts.row = 5;
	ProUIDialogPushbuttonAdd(dialogName, exportToStp, &gridOpts);
	ProStringToWstring(label[0], exportToStp);
	ProUIPushbuttonTextSet(dialogName, exportToStp, label[0]);
	ProUIPushbuttonActivateActionSet(dialogName, exportToStp, exportToStpAction, NULL);

	ProUIDialogActivate(dialogName, &exit_status);
	ProUIDialogDestroy(dialogName);
}

void printMessage(string message)
{
	ProMessageDisplay(msgFile, strToCharArr(message));
}

static uiCmdAccessState AccessAvailable(uiCmdAccessMode access_mode)
{
	return (ACCESS_AVAILABLE);
}

char* strToCharArr(string str)
{
	char* cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());
	return cstr;
}

void initializeMsgFile()
{
	char MSGFIL[] = "creobatch.txt";
	ProStringToWstring(msgFile, MSGFIL);
}

void cancelAction(char* dialog, char* component, ProAppData data)
{
	ProUIDialogExit(dialog, 0);
}

void exportToPdfAction(char* dialog, char* component, ProAppData data)
{
	parseTextArea();

	bool success = true;
	ProError err = PRO_TK_NO_ERROR;
	int newWindowId;
	ProName drwName;
	ProPath filenameWithPath;
	ProMdl drawingToExport = NULL;
	ProPDFOptions pdfOptions;
	wstring notExportedNumbers;

	ProPDFoptionsAlloc(&pdfOptions);
	ProPDFoptionsIntpropertySet(pdfOptions, PRO_PDFOPT_COLOR_DEPTH, PRO_PDF_CD_MONO);
	ProPDFoptionsBoolpropertySet(pdfOptions, PRO_PDFOPT_LAUNCH_VIEWER, PRO_B_FALSE);
	ProUIDialogExit(dialogName, 0);

	for (int i = 0; i < numbers.size(); i++)
	{
		int numLen = 0;
		ProWstringLengthGet(numbers[i], &numLen);
		ProDirectoryCurrentGet(filenameWithPath);
		err = ProMdlRetrieve(numbers[i], PRO_MDL_DRAWING, &drawingToExport);
		if (err != PRO_TK_NO_ERROR)
		{
			success = false;
			notExportedNumbers += numbers[i];
			notExportedNumbers += L", ";
			continue;
		}
		ProMdlNameGet(drawingToExport, drwName);
		ProObjectwindowCreate(drwName, PRO_DRAWING, &newWindowId);
		ProWindowCurrentSet(newWindowId);
		ProWindowActivate(newWindowId);
		ProWstringConcatenate(numbers[i], filenameWithPath, numLen);
		ProWstringConcatenate((wchar_t*)L".pdf", filenameWithPath, 4);
		ProPDFExport(drawingToExport, filenameWithPath, pdfOptions);
		ProWindowCurrentClose();
		ProMdlErase(drawingToExport);
	}
	ProPDFoptionsFree(pdfOptions);
	if (success)
	{
		ProMessageDisplay(msgFile, strToCharArr("summary success"));
	}
	else
	{
		notExportedNumbers.erase(notExportedNumbers.end() - 2);
		ProMessageDisplay(msgFile, strToCharArr("summary with errors"), notExportedNumbers.c_str());
	}
}

void exportToDxfAction(char* dialog, char* component, ProAppData data)
{
	parseTextArea();

	bool success = true;
	ProError err = PRO_TK_NO_ERROR;
	ProPath filenameWithPath;
	ProMdl drawingToExport = NULL;
	wstring notExportedNumbers;

	for (int i = 0; i < numbers.size(); i++)
	{
		int numLen = 0;
		ProWstringLengthGet(numbers[i], &numLen);
		ProDirectoryCurrentGet(filenameWithPath);
		err = ProMdlRetrieve(numbers[i], PRO_MDL_DRAWING, &drawingToExport);
		if (err != PRO_TK_NO_ERROR)
		{
			success = false;
			notExportedNumbers += numbers[i];
			notExportedNumbers += L", ";
			continue;
		}
		ProWstringConcatenate(numbers[i], filenameWithPath, numLen);
		ProWstringConcatenate((wchar_t*)L".dxf", filenameWithPath, 4);
		Pro2dExport(PRO_DXF_FILE, filenameWithPath, drawingToExport, NULL);
		ProMdlErase(drawingToExport);
	}
	if (success)
	{
		ProMessageDisplay(msgFile, strToCharArr("summary success"));
	}
	else
	{
		notExportedNumbers.erase(notExportedNumbers.end() - 2);
		ProMessageDisplay(msgFile, strToCharArr("summary with errors"), notExportedNumbers.c_str());
	}
}

void exportToStpAction(char* dialog, char* component, ProAppData data)
{
}

void parseTextArea()
{
	numbers.clear();

	wchar_t* wstr;
	ProUITextareaValueGet(dialogName, textArea, &wstr);
	memcpy(textAreaValue, wstr, sizeof(wchar_t) * textAreaLength);
	ProArrayFree((ProArray*)&wstr);

	wchar_t* line;
	wchar_t* nextToken;
	line = wcstok_s(textAreaValue, L" .,;\r\n\t", &nextToken);
	int length = 0;
	while (line != NULL)
	{
		ProWstringLengthGet(line, &length);
		if (length > 5)
			numbers.push_back(line);
		line = wcstok_s(NULL, L" .,;\r\n\t", &nextToken);
	}
}

void textAction(char* dialog, char* component, ProAppData data)
{
}