unit Main;

interface

uses
  Child, SysUtils, Windows, Messages, Classes, Graphics, Controls,
  Forms, Dialogs, StdCtrls, Buttons, ExtCtrls, Menus, ComCtrls, ExtDlgs,
  ActnList;

type
  TMainForm = class(TForm)

    MainMenu: TMainMenu;
    FileNewItem: TMenuItem;
    FileOpenItem: TMenuItem;
    FileSaveItem: TMenuItem;
    FileSaveAsItem: TMenuItem;
    FilePrintItem: TMenuItem;
    FilePrintSetupItem: TMenuItem;
    FileExitItem: TMenuItem;
    EditUndoItem: TMenuItem;
    WindowTileItem: TMenuItem;
    WindowCascadeItem: TMenuItem;
    WindowArrangeItem: TMenuItem;
    HelpAboutItem: TMenuItem;
    StatusLine: TStatusBar;
    OpenDialog: TOpenDialog;
    PrintDialog: TPrintDialog;
    PrintSetupDialog: TPrinterSetupDialog;
    SpeedBar: TPanel;
    SpeedButton1: TSpeedButton;  { &New }
    SpeedButton2: TSpeedButton;
    SpeedButtonSave: TSpeedButton;  { &Undo }
    SpeedButton5: TSpeedButton;  { &Tile }
    SpeedButton6: TSpeedButton;  { &Cascade }
    SpeedButton7: TSpeedButton;  { &Arrange All }

    NormalPaintBox: TPaintBox;
    SaveDialog: TSaveDialog;
    FileSavePictureItem: TMenuItem;
    SavePictureDialog: TSavePictureDialog;
    ActionList1: TActionList;
    ActionSavePicture: TAction;
    N3: TMenuItem;
    ActionImportFromSyntax: TAction;
    FileImportItem: TMenuItem;
    ImportDialog: TOpenDialog;
    GrammarMenu: TMenuItem;
    GrammarMacroOpenedItem: TMenuItem;
    GrammarOpenAllMacroItem: TMenuItem;
    GrammarCloseAllDefinitionItem: TMenuItem;
    ActionImportFromGEdit: TAction;
    ActionImportFromGEdit1: TMenuItem;
    DebugMemo: TMemo;
    SpeedButton3: TSpeedButton;
    SpeedButton4: TSpeedButton;
    SpeedButtonSaveAs: TSpeedButton;
    FileCloseItem: TMenuItem;
    SpeedButton8: TSpeedButton;
    EditRedoItem: TMenuItem;
    SpeedButton9: TSpeedButton;
    SpeedButton10: TSpeedButton;
    SpeedButtonAnalyze: TSpeedButton;
    TransformationMenu: TMenuItem;
    LeftEliminationItem: TMenuItem;
    RightEliminationItem: TMenuItem;
    SubstituteItem: TMenuItem;
    Extract: TMenuItem;
    SpeedButton11: TSpeedButton;
    ActionExtract: TAction;
    RemoveUselessNonterms: TAction;
    SpeedButton12: TSpeedButton;
    SpeedButton13: TSpeedButton;
    procedure FormCreate(Sender: TObject);
    procedure ShowHint(Sender: TObject);
    procedure FileNew(Sender: TObject);
    procedure FileOpen(Sender: TObject);
    procedure FileSave(Sender: TObject);
    procedure FileSaveAs(Sender: TObject);
    procedure FilePrint(Sender: TObject);
    procedure FilePrintSetup(Sender: TObject);
    procedure FileExit(Sender: TObject);
    procedure EditUndo(Sender: TObject);
    procedure EditCut(Sender: TObject);
    procedure EditCopy(Sender: TObject);
    procedure EditPaste(Sender: TObject);
    procedure WindowTile(Sender: TObject);
    procedure WindowCascade(Sender: TObject);
    procedure WindowArrange(Sender: TObject);
    procedure HelpContents(Sender: TObject);
    procedure HelpSearch(Sender: TObject);
    procedure HelpHowToUse(Sender: TObject);
    procedure HelpAbout(Sender: TObject);
    procedure EnableMenuItems;
    procedure DisableMenuItems;
    procedure ActionSavePictureExecute(Sender: TObject);
    procedure ActionImportFromSyntaxExecute(Sender: TObject);
    procedure GrammarMacroOpenedItemClick(Sender: TObject);
    procedure GrammarOpenAllMacroItemClick(Sender: TObject);
    procedure GrammarCloseAllDefinitionItemClick(Sender: TObject);
    procedure ActionImportFromGEditExecute(Sender: TObject);
    procedure SpeedButton3Click(Sender: TObject);
    procedure SpeedButton4Click(Sender: TObject);
    procedure FileCloseItemClick(Sender: TObject);
    procedure SpeedButton8Click(Sender: TObject);
    procedure EditRedoItemClick(Sender: TObject);
    procedure SpeedButtonAnalyzeClick(Sender: TObject);
    procedure ActionExtractExecute(Sender: TObject);
    procedure RemoveUselessNontermsExecute(Sender: TObject);
    procedure SpeedButton12Click(Sender: TObject);
    procedure SpeedButton13Click(Sender: TObject);

  private
    function getMacroOpened:boolean;
    procedure CreateMDIChild(const Name: string);
    procedure ImportFromGEditMDIChild(const Name: string);
    procedure ImportFromSyntaxMDIChild(const Name: string);
  public
    property MacroOpened:boolean read getMacroOpened;

    procedure print(str :String);
    procedure println(str :String);

  end;

var
  MainForm: TMainForm;
  NormalSizeCanvas: TCanvas;

implementation
uses RegularExpression, UndoRedo, Analyzer;
{$R *.DFM}


procedure TMainForm.print(str :String);
begin
  DebugMemo.Text:=DebugMemo.Text+str;
end;

procedure TMainForm.println(str :String);
begin
  DebugMemo.Text:=DebugMemo.Text+str+#13+#10;
end;

procedure TMainForm.FormCreate(Sender: TObject);
begin
  Application.OnHint := ShowHint;
  NormalSizeCanvas:= NormalPaintBox.Canvas;
end;

procedure TMainForm.ShowHint(Sender: TObject);
begin
  StatusLine.SimpleText := Application.Hint;
end;

procedure TMainForm.CreateMDIChild(const Name: string);
var
  Child: TChildForm;
  a: TSelMas;
begin
//   create a new MDI child window
  SetLength(a,0);

  Child := TChildForm.Create(Application);
  Child.Caption := Name;
  if FileExists(Name) then
    Child.Grammar.Load(Name)
  else
    Child.Grammar.fillNew;
  Child.SetActiveNonTerminal(0);
  Child.UndoRedo.AddState(Child.NonTerminalListBox,Child.NonTerminalListBox.ItemIndex,a);
end;
procedure TMainForm.ImportFromSyntaxMDIChild(const Name: string);
var
  Child: TChildForm;
  Capt:string;
begin
//   create a new MDI child window
  Child := TChildForm.Create(Application);
  if FileExists(Name) then
    Child.Grammar.ImportFromSyntax(Name)
  else
    Child.Grammar.fillNew;
  Capt := Name;
  Capt[Length(Capt)]:='w';
  Child.Caption:=capt;
  Child.SetActiveNonTerminal(0);
end;

procedure TMainForm.ImportFromGEditMDIChild(const Name: string);
var
  Child: TChildForm;
  Capt:string;
begin
//   create a new MDI child window
  Child := TChildForm.Create(Application);
  if FileExists(Name) then
    Child.Grammar.ImportFromGEdit(Name)
  else
    Child.Grammar.fillNew;
  Capt := Name;
  Capt[Length(Capt)]:='w';
  Child.Caption:=capt;
  Child.SetActiveNonTerminal(0);
end;

{
procedure TMainForm.CreateMDIChildFromFile(const Name: string);
var
  Child: TChildForm;
begin
//   create a new MDI child window
  Child := TChildForm.Create(Application);
  Child.Caption := Name;
  Child.Grammar.Load(Name);
  Child.SetActiveNonTerminal(0);
end;
}
procedure TMainForm.FileNew(Sender: TObject);
begin
  if MDIChildCount=0 then EnableMenuItems;
  CreateMDIChild('NONAME');
end;
procedure TMainForm.EnableMenuItems;
begin
  if not(FileSaveItem.Enabled) then begin
    FilePrintItem.Enabled:=true;
    GrammarOpenAllMacroItem.Enabled:=true;
    GrammarCloseAllDefinitionItem.Enabled:=true;
    FileSaveItem.Enabled:=true;
    FileSavePictureItem.Enabled:=true;
    FileSaveAsItem.Enabled:=true;
    FileCloseItem.Enabled:=true;
    SpeedButtonSave.Enabled:=true;
    SpeedButtonSaveAs.Enabled:=true;
  end;
end;

procedure TMainForm.DisableMenuItems;
begin
  if MDIChildCount<=1 then begin
    FilePrintItem.Enabled:=false;
    FileSaveItem.Enabled:=false;
    GrammarOpenAllMacroItem.Enabled:=false;
    GrammarCloseAllDefinitionItem.Enabled:=false;
    FileSavePictureItem.Enabled:=false;
    FileSaveAsItem.Enabled:=false;
    FileCloseItem.Enabled:=false;
    SpeedButtonSave.Enabled:=false;
    SpeedButtonSaveAs.Enabled:=false;
  end;
end;


procedure TMainForm.FileOpen(Sender: TObject);
begin
  if OpenDialog.Execute then
  begin
    { Add code to open OpenDialog.FileName }
    if MDIChildCount=0 then EnableMenuItems;
    CreateMDIChild(OpenDialog.FileName);
  end;
end;

procedure TMainForm.ActionImportFromSyntaxExecute(Sender: TObject);
begin
  if ImportDialog.Execute then
  try
    { Add code to open OpenDialog.FileName }
    ImportFromSyntaxMDIChild(ImportDialog.FileName);
    EnableMenuItems;
  except
    if ActiveMDIChild<>nil then (TChildForm(ActiveMDIChild)).Close;
    MessageDlg('FileOpen error!'+chr(13)+'Not syntax format.',mtError,[mbOk],0);
  end;
end;

procedure TMainForm.ActionImportFromGEditExecute(Sender: TObject);
begin
  if ImportDialog.Execute then
  try
    { Add code to open OpenDialog.FileName }
    ImportFromGEditMDIChild(ImportDialog.FileName);
    EnableMenuItems;
  except
    if ActiveMDIChild<>nil then (TChildForm(ActiveMDIChild)).Close;
    MessageDlg('FileOpen error!'+chr(13)+'Not GEdit format.',mtError,[mbOk],0);
  end;
end;
procedure TMainForm.FileSave(Sender: TObject);
{  function BeginWithNoName(const str:String):boolean;
  var i:integer;
    NONAME:String;
  begin
    NONAME:='NONAME';
    result:=true;
    for i:=1 to length(NONAME)do
      if str[i]<>NONAME[i] then result:=false;
  end;                              }
begin
  if ActiveMDIChild.Caption='NONAME' then begin
    FileSaveAs(Sender);
  end else begin
    { Add code to save current file under current name }
    TChildForm(ActiveMDIChild).Save;
  end;
end;
procedure TMainForm.FileSaveAs(Sender: TObject);
begin
  SaveDialog.FileName:=ActiveMDIChild.Caption;
  if SaveDialog.Execute then
  begin
    { Add code to save current file under SaveDialog.FileName }
    ActiveMDIChild.Caption:=SaveDialog.FileName;
    TChildForm(ActiveMDIChild).Save;
  end
end;
procedure TMainForm.ActionSavePictureExecute(Sender: TObject);
begin
  if SavePictureDialog.Execute then
  begin
    { Add code to save current file under SavePictureDialog.FileName }
    TChildForm(ActiveMDIChild).SavePicture(SavePictureDialog.FileName);
  end
end;

procedure TMainForm.FilePrint(Sender: TObject);
begin
  if PrintDialog.Execute then
  begin
    { Add code to print current file }
    TChildForm(ActiveMDIChild).PrintOffScreen;
  end;
end;

procedure TMainForm.FilePrintSetup(Sender: TObject);
begin
  PrintSetupDialog.Execute;
end;

procedure TMainForm.FileExit(Sender: TObject);
begin
  Close;
end;

procedure TMainForm.EditUndo(Sender: TObject);
var ind: integer;
    a: TSelMas;
begin
  if ActiveMDIChild<>nil then begin
                              (TChildForm(ActiveMDIChild)).UndoRedo.StepBack((TChildForm(ActiveMDIChild)).grammar,(TChildForm(ActiveMDIChild)).NonTerminalListBox,ind,a);
                              (TChildForm(ActiveMDIChild)).SetActiveNonTerminal(ind);
                              (TChildForm(ActiveMDIChild)).SetSelMas(a);
                              (TChildForm(ActiveMDIChild)).ReDrawOffScreen;
                              (TChildForm(ActiveMDIChild)).ClientAreaPaint(nil);
                              end;
end;

procedure TMainForm.EditCut(Sender: TObject);
begin
  { Add code to perform Edit Cut }
end;

procedure TMainForm.EditCopy(Sender: TObject);
begin
  { Add code to perform Edit Copy }
end;

procedure TMainForm.EditPaste(Sender: TObject);
begin
  { Add code to perform Edit Paste }
end;

procedure TMainForm.WindowTile(Sender: TObject);
begin
  Tile;
end;

procedure TMainForm.WindowCascade(Sender: TObject);
begin
  Cascade;
end;

procedure TMainForm.WindowArrange(Sender: TObject);
begin
  ArrangeIcons;
end;

procedure TMainForm.HelpContents(Sender: TObject);
begin
  Application.HelpCommand(HELP_CONTENTS, 0);
end;

procedure TMainForm.HelpSearch(Sender: TObject);
const
  EmptyString: PChar = '';
begin
  Application.HelpCommand(HELP_PARTIALKEY, Longint(EmptyString));
end;

procedure TMainForm.HelpHowToUse(Sender: TObject);
begin
  Application.HelpCommand(HELP_HELPONHELP, 0);
end;

procedure TMainForm.HelpAbout(Sender: TObject);
begin
  { Add code to show program's About Box }
end;

procedure TMainForm.GrammarMacroOpenedItemClick(Sender: TObject);
begin
  GrammarMacroOpenedItem.checked:=not GrammarMacroOpenedItem.checked;
end;

function TMainForm.getMacroOpened:boolean;
begin
  result:=GrammarMacroOpenedItem.checked;
end;
procedure TMainForm.GrammarOpenAllMacroItemClick(Sender: TObject);
begin
  TChildForm(ActiveMDIChild).ActionOpenAllMacroExecute(Sender);
end;

procedure TMainForm.GrammarCloseAllDefinitionItemClick(Sender: TObject);
begin
  TChildForm(ActiveMDIChild).ActionCloseAllDefinitionExecute(Sender);
end;

procedure TMainForm.SpeedButton3Click(Sender: TObject);
begin
try
  SpeedButton3.Enabled:=false;
  if ActiveMDIChild<>nil then (TChildForm(ActiveMDIChild)).EllLeft;
finally
  SpeedButton3.Enabled:=true;
end;
end;

procedure TMainForm.SpeedButton4Click(Sender: TObject);
begin
try
  SpeedButton4.Enabled:=false;
  if ActiveMDIChild<>nil then (TChildForm(ActiveMDIChild)).EllRight;
finally
  SpeedButton4.Enabled:=true;
end;
end;

procedure TMainForm.FileCloseItemClick(Sender: TObject);
begin
  if ActiveMDIChild<>nil then (TChildForm(ActiveMDIChild)).Close;
end;

procedure TMainForm.SpeedButton8Click(Sender: TObject);
begin
if ActiveMDIChild<>nil then begin
                            (TChildForm(ActiveMDIChild)).Substitute;
                            end;
end;

procedure TMainForm.EditRedoItemClick(Sender: TObject);
var ind: integer;
    a: TSelMas;
begin
  if ActiveMDIChild<>nil then begin
                              (TChildForm(ActiveMDIChild)).UndoRedo.StepForward((TChildForm(ActiveMDIChild)).grammar,(TChildForm(ActiveMDIChild)).NonTerminalListBox,ind,a);
                              (TChildForm(ActiveMDIChild)).SetActiveNonTerminal(ind);
                              (TChildForm(ActiveMDIChild)).SetSelMas(a);
                              (TChildForm(ActiveMDIChild)).ReDrawOffScreen;
                              (TChildForm(ActiveMDIChild)).ClientAreaPaint(nil);
                              end;
end;

procedure TMainForm.SpeedButtonAnalyzeClick(Sender: TObject);
begin
if ActiveMDIChild<>nil then begin
                            AnalyzeForm.Show;
                            AnalyzeForm.ShowAll((TChildForm(ActiveMDIChild)).ActiveListBox);
                            end;
end;

procedure TMainForm.ActionExtractExecute(Sender: TObject);
begin
if ActiveMDIChild<>nil then begin
                              (TChildForm(ActiveMDIChild)).ExtractRule;
                              //(TChildForm(ActiveMDIChild)).ReDrawOffScreen;
                              (TChildForm(ActiveMDIChild)).ClientAreaPaint(nil);
                            end;
end;

procedure TMainForm.RemoveUselessNontermsExecute(Sender: TObject);
begin
if ActiveMDIChild<>nil then begin
                              (TChildForm(ActiveMDIChild)).RemoveUselessNonterms;
                              //(TChildForm(ActiveMDIChild)).ReDrawOffScreen;
                              (TChildForm(ActiveMDIChild)).ClientAreaPaint(nil);
                            end;
end;

procedure TMainForm.SpeedButton12Click(Sender: TObject);
begin
try
  SpeedButton12.Enabled:=false;
  if ActiveMDIChild<>nil then (TChildForm(ActiveMDIChild)).EllBoth;
finally
  SpeedButton12.Enabled:=true;
end;
end;

procedure TMainForm.SpeedButton13Click(Sender: TObject);
begin
try
  SpeedButton13.Enabled:=false;
  if ActiveMDIChild<>nil then (TChildForm(ActiveMDIChild)).Minimize;
finally
  SpeedButton13.Enabled:=true;
end;
end;

end.
