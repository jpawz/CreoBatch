/**
Creo batch tool.
Actions:
1. Convert drw to pdf
2. Convert drw to dxf
3. Convert drw to dwg
 */

#include <ProCore.h>
#include <ProDrawing.h>
#include <ProIntf3Dexport.h>
#include <ProMenuBar.h>
#include <ProMessage.h>
#include <ProUICmd.h>
#include <ProUIDialog.h>
#include <ProUIMessage.h>
#include <ProUIPushbutton.h>
#include <ProUITextarea.h>
#include <ProUtil.h>
#include <ProWindows.h>
#include <ProWstring.h>
#include <ProWTUtils.h>
#include <ProPDF.h>
#include <vector>

using namespace std;

static uiCmdAccessState AccessAvailable(uiCmdAccessMode);
void makeDialogWindow();
void initializeMsgFile();
void parseTextArea();

void cancelAction(char*, char*, ProAppData);
void exportToPdfAction(char*, char*, ProAppData);
void exportToDxfAction(char*, char*, ProAppData);
void exportToDwgAction(char*, char*, ProAppData);
void exportToStpAction(char*, char*, ProAppData);
void getAllDrwFromWorkspaceAction(char*, char*, ProAppData);
void export2dDrawing(ProImportExportFile, wchar_t*);
void textAction(char*, char*, ProAppData);
void summary(bool);

ProFileName msgFile;
char dialogName[] = { "creobatch" };
char separator[] = { "Separator" };
char cancel[] = { "Cancel" };
char exportToPdf[] = { "Eksport do PDF" };
char exportToDxf[] = { "Eksport do DXF" };
char exportToDwg[] = { "Eksport do DWG" };
char exportToStp[] = { "Eksport do STP" };
char getAllDrw[] = { "<- *.drw z workspace" };
char textArea[] = { "TextArea" };

vector<wchar_t*> numbers;
const int textAreaLength = 2048;
static wchar_t textAreaValue[textAreaLength];

static wstring notExportedNumbers;
static bool success = true;

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
	ProUITextareaHelptextSet(dialogName, textArea, (wchar_t*)L"Numery rozdzielone spacjami, przecinkami, kropkami, średnikami lub w nowych wierszach. \nNumery krótsze niż 4 znaki są ignorowane.");

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
	ProUIDialogPushbuttonAdd(dialogName, getAllDrw, &gridOpts);
	ProStringToWstring(label[0], getAllDrw);
	ProUIPushbuttonTextSet(dialogName, getAllDrw, label[0]);
	ProUIPushbuttonActivateActionSet(dialogName, getAllDrw, getAllDrwFromWorkspaceAction, NULL);
	ProUIPushbuttonHelptextSet(dialogName, getAllDrw, (wchar_t*)L"wkleja wszyskie numery rysunków z worksapce");

	gridOpts.row = 4;
	ProUIDialogSeparatorAdd(dialogName, separator, &gridOpts);

	gridOpts.row = 5;
	ProUIDialogPushbuttonAdd(dialogName, exportToPdf, &gridOpts);
	ProStringToWstring(label[0], exportToPdf);
	ProUIPushbuttonTextSet(dialogName, exportToPdf, label[0]);
	ProUIPushbuttonActivateActionSet(dialogName, exportToPdf, exportToPdfAction, NULL);
	ProUIPushbuttonHelptextSet(dialogName, exportToPdf, (wchar_t*)L"zapisze rysunki do plikow PDF w katalogu roboczym");

	gridOpts.row = 6;
	ProUIDialogPushbuttonAdd(dialogName, exportToDxf, &gridOpts);
	ProStringToWstring(label[0], exportToDxf);
	ProUIPushbuttonTextSet(dialogName, exportToDxf, label[0]);
	ProUIPushbuttonActivateActionSet(dialogName, exportToDxf, exportToDxfAction, NULL);
	ProUIPushbuttonHelptextSet(dialogName, exportToDxf, (wchar_t*)L"zapisze rysunki do plikow DXF w katalogu roboczym");

	gridOpts.row = 7;
	ProUIDialogPushbuttonAdd(dialogName, exportToStp, &gridOpts);
	ProStringToWstring(label[0], exportToStp);
	ProUIPushbuttonTextSet(dialogName, exportToStp, label[0]);
	ProUIPushbuttonActivateActionSet(dialogName, exportToStp, exportToStpAction, NULL);
	ProUIPushbuttonHelptextSet(dialogName, exportToStp, (wchar_t*)L"zapisze modele do plikow STP w katalogu roboczym");

	ProUIDialogActivate(dialogName, &exit_status);
	ProUIDialogDestroy(dialogName);
}

static uiCmdAccessState AccessAvailable(uiCmdAccessMode access_mode)
{
	return (ACCESS_AVAILABLE);
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

void getAllDrwFromWorkspaceAction(char* dialog, char* component, ProAppData data)
{
	wchar_t* workspace;
	wchar_t* alias;
	ProServerActiveGet(&alias);
	ProServerWorkspaceGet(alias, &workspace);
	wstring workspacePath = L"wtws://";
	workspacePath.append(alias);
	workspacePath.append(L"/");
	workspacePath.append(workspace);

	ProPath* fileList, * dirList;
	ProArrayAlloc(0, sizeof(ProPath), 1, (ProArray*)&fileList);
	ProArrayAlloc(0, sizeof(ProPath), 1, (ProArray*)&dirList);
	ProFilesList((wchar_t*)workspacePath.c_str(), (wchar_t*)L"*.drw", PRO_FILE_LIST_LATEST_SORTED, &fileList, &dirList);

	int nFiles;
	ProArraySizeGet((ProArray)fileList, &nFiles);
	wstring drwFiles;
	for (int i = 0; i < nFiles; i++)
	{
		wstring line(fileList[i]);
		size_t pos = line.find_last_of(L'/');
		drwFiles.append(fileList[i], pos + 1, wstring::npos);
		drwFiles.append(L"\n");
	}

	ProUITextareaValueSet(dialogName, textArea, (wchar_t*)drwFiles.c_str());
}

void exportToPdfAction(char* dialog, char* component, ProAppData data)
{
	parseTextArea();

	success = true;
	ProError err = PRO_TK_NO_ERROR;
	int newWindowId;
	int oldWindowId;
	ProName drwName;
	ProPath filenameWithPath;
	ProMdl drawingToExport = NULL;
	ProPDFOptions pdfOptions;

	ProPDFoptionsAlloc(&pdfOptions);
	ProPDFoptionsIntpropertySet(pdfOptions, PRO_PDFOPT_COLOR_DEPTH, PRO_PDF_CD_MONO);
	ProPDFoptionsBoolpropertySet(pdfOptions, PRO_PDFOPT_LAUNCH_VIEWER, PRO_B_FALSE);
	ProUIDialogExit(dialogName, 0);

	ProWindowCurrentGet(&oldWindowId);

	notExportedNumbers.clear();

	for (int i = 0; i < numbers.size(); i++)
	{
		int numLen = 0;
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
		ProWstringLengthGet(numbers[i], &numLen);
		ProWstringConcatenate(numbers[i], filenameWithPath, numLen);
		ProWstringConcatenate((wchar_t*)L".pdf", filenameWithPath, 4);
		ProPDFExport(drawingToExport, filenameWithPath, pdfOptions);
		//ProWindowCurrentClose();
		ProMdlErase(drawingToExport);
	}

	ProPDFoptionsFree(pdfOptions);
	ProWindowActivate(oldWindowId);

	summary(success);
}

void exportToDxfAction(char* dialog, char* component, ProAppData data)
{
	parseTextArea();

	export2dDrawing(PRO_DXF_FILE, (wchar_t*)L".dxf");
}

void exportToDwgAction(char* dialog, char* component, ProAppData data)
{
	parseTextArea();

	export2dDrawing(PRO_DWG_FILE, (wchar_t*)L".dwg");
}

void exportToStpAction(char* dialog, char* component, ProAppData data)
{
	parseTextArea();

	success = true;
	ProError err = PRO_TK_NO_ERROR;
	ProPath filenameWithPath;
	ProMdl modelToExport = NULL;

	ProOutputBrepRepresentation brep_flags;
	ProOutputInclusion inclusion;
	ProOutputLayerOptions layer_options;

	ProOutputBrepRepresentationAlloc(&brep_flags);
	ProOutputBrepRepresentationFlagsSet(brep_flags,
		PRO_B_FALSE,
		PRO_B_TRUE,
		PRO_B_FALSE,
		PRO_B_FALSE);

	ProOutputInclusionAlloc(&inclusion);
	ProOutputInclusionFlagsSet(inclusion,
		PRO_B_TRUE,
		PRO_B_FALSE,
		PRO_B_FALSE);

	ProOutputLayerOptionsAlloc(&layer_options);
	ProOutputLayerOptionsAutoidSet(layer_options, PRO_B_TRUE);

	notExportedNumbers.clear();

	for (int i = 0; i < numbers.size(); i++)
	{
		int numLen = 0;
		ProWstringLengthGet(numbers[i], &numLen);

		ProDirectoryCurrentGet(filenameWithPath);
		ProWstringConcatenate(numbers[i], filenameWithPath, numLen);
		ProWstringConcatenate((wchar_t*)L".stp", filenameWithPath, 4);

		err = ProMdlRetrieve(numbers[i], PRO_MDL_PART, &modelToExport);
		if (err == PRO_TK_NO_ERROR)
		{
			ProIntf3DFileWrite((ProPart)modelToExport, PRO_INTF_EXPORT_STEP, filenameWithPath, PRO_OUTPUT_ASSEMBLY_SINGLE_FILE, NULL, brep_flags, inclusion, layer_options);
			ProMdlErase(modelToExport);
			continue;
		}

		err = ProMdlRetrieve(numbers[i], PRO_MDL_ASSEMBLY, &modelToExport);
		if (err == PRO_TK_NO_ERROR)
		{
			ProIntf3DFileWrite((ProAssembly)modelToExport, PRO_INTF_EXPORT_STEP, filenameWithPath, PRO_OUTPUT_ASSEMBLY_SINGLE_FILE, NULL, brep_flags, inclusion, layer_options);
			ProMdlErase(modelToExport);
			continue;
		}

		success = false;
		notExportedNumbers += numbers[i];
		notExportedNumbers += L", ";
	}
	summary(success);
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
		if (length > 3)
			numbers.push_back(line);
		line = wcstok_s(NULL, L" .,;\r\n\t", &nextToken);
	}
}

void textAction(char* dialog, char* component, ProAppData data)
{
}

void export2dDrawing(ProImportExportFile importExportFile, wchar_t* extension)
{
	success = true;
	ProError err = PRO_TK_NO_ERROR;
	ProPath filenameWithPath;
	ProMdl drawingToExport = NULL;

	notExportedNumbers.clear();

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

		int sheets = 0;
		wchar_t num[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9'};
		wchar_t underscore = L'_';
		ProDrawingSheetsCount((ProDrawing)drawingToExport, &sheets);
		if (sheets > 1) {
			for (int j = 0; j < sheets; j++) {
				ProDrawingCurrentSheetSet((ProDrawing)drawingToExport, j + 1);
				ProWstringConcatenate(numbers[i], filenameWithPath, numLen);
				ProWstringConcatenate(&underscore, filenameWithPath, 1);
				ProWstringConcatenate(&num[j], filenameWithPath, 1);
				ProWstringConcatenate(extension, filenameWithPath, 4);
				Pro2dExport(importExportFile, filenameWithPath, drawingToExport, NULL);
				ProDirectoryCurrentGet(filenameWithPath);
			}
		}
		else {
			ProWstringConcatenate(numbers[i], filenameWithPath, numLen);
			ProWstringConcatenate(extension, filenameWithPath, 4);
			Pro2dExport(importExportFile, filenameWithPath, drawingToExport, NULL);
		}
		ProMdlErase(drawingToExport);
	}
	summary(success);
}

void summary(bool status)
{
	ProUIMessageButton* buttons;
	ProUIMessageButton userChoice;
	ProArrayAlloc(1, sizeof(ProUIMessageButton),
		1, (ProArray*)&buttons);
	buttons[0] = PRO_UI_MESSAGE_OK;
	if (status)
	{
		ProUIMessageDialogDisplay(PROUIMESSAGE_INFO, (wchar_t*)L"Koniec", (wchar_t*)L"Wszystkie numery zapisano.", buttons, PRO_UI_MESSAGE_OK, &userChoice);
	}
	else
	{
		wstring message = L"Tych numerów nie zapisano:\n";
		int counter = 0;
		for (int i = 0; i < notExportedNumbers.size() - 1; i++)
		{
			if ((notExportedNumbers.at(i) == L',') && (notExportedNumbers.at(i + 1) == L' '))
			{
				counter++;
				if (counter % 10 == 0)
				{
					notExportedNumbers.replace(i, 2, L"\n");
				}
			}
		}
		message += notExportedNumbers;
		ProUIMessageDialogDisplay(PROUIMESSAGE_WARNING, (wchar_t*)L"Błędy", (wchar_t*)message.c_str(), buttons, PRO_UI_MESSAGE_OK, &userChoice);
	}
}