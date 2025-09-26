unit Child;

interface

uses
  RegularExpression, UndoRedo, Analyzer,
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ExtCtrls, Menus, ComCtrls, ActnList;
const
  cMinPaintBoxHeight=0;
  cMinPaintBoxWidth=0;
  cNone=0;
  cMove=1;
  cContur=2;
type
  TChildForm = class(TForm)
    Dictionary: TGroupBox;
    ScrollBox1: TScrollBox;
    ClientArea: TPaintBox;
    Panel1: TPanel;
    EditArea: TEdit;
    BuildButton: TButton;
    PageControl1: TPageControl;
    NonTerminals: TTabSheet;
    NonTerminalListBox: TListBox;
    Terminals: TTabSheet;
    TerminalListBox: TListBox;
    Semantics: TTabSheet;
    Macro: TTabSheet;
    AuxilaryNotionsListBox: TListBox;
    SemanticListBox: TListBox;
    PopupMenu1: TPopupMenu;
    AddMacro: TMenuItem;
    AddNonTerminal: TMenuItem;
    AddSemantic: TMenuItem;
    ActionList1: TActionList;
    ActionAddMacro: TAction;
    ActionAddNonTerminal: TAction;
    ActionAddSemantic: TAction;
    N1: TMenuItem;
    ActionEditActiveName: TAction;
    EditActiveNameItem: TMenuItem;
    ActionOpenAllMacro: TAction;
    ActionCloseAllDefinition: TAction;
    PopupMenu2: TPopupMenu;
    ActionChangeBitmapSize: TAction;
    OpenAllMacro: TMenuItem;
    CloseAllDefinition: TMenuItem;
    N2: TMenuItem;
    ChangeBitmapSize: TMenuItem;
    ActionAddExtendedPoint: TAction;
    ActionAddExtendedPoint1: TMenuItem;
    Splitter1: TSplitter;

    procedure FormCreate(Sender: TObject);
    procedure ClientAreaPaint(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure BuildButtonClick(Sender: TObject);
    procedure NonTerminalListBoxClick(Sender: TObject);
    procedure AuxilaryNotionsListBoxClick(Sender: TObject);

    procedure ActionAddMacroExecute(Sender: TObject);
    procedure ActionAddNonTerminalExecute(Sender: TObject);
    procedure ActionAddSemanticExecute(Sender: TObject);
    procedure ActionEditActiveNameExecute(Sender: TObject);
    procedure ActionOpenAllMacroExecute(Sender: TObject);
    procedure ActionCloseAllDefinitionExecute(Sender: TObject);
    procedure ClientAreaMouseUp(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure ClientAreaMouseDown(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure ClientAreaMouseMove(Sender: TObject; Shift: TShiftState; X,
      Y: Integer);
    procedure ActionChangeBitmapSizeExecute(Sender: TObject);
    procedure ActionAddExtendedPointExecute(Sender: TObject);
  private
    m_ActiveObject: TNTListItem;
    m_Contur:TRect;
    { Private declarations }
    function getSemanticTable:TStrings;
  public
    { Public declarations }
    offScreen :TBitmap;
    Grammar: TGrammar;
    moveDrawObjects:integer;
    ActiveListBox:TListBox;
    UndoRedo:TUndoRedo;
    Procedure SetActiveNonTerminal(index:Integer);
    Procedure SetActiveMacro(index:Integer);

    Procedure SetActive;
    Procedure Save;
    Procedure SavePicture(const FileName:string);
    Procedure PrintOffScreen;
    Procedure reDrawOffScreen;

    property SemanticTable:TStrings read getSemanticTable;

    Procedure EllLeft;
    Procedure EllRight;
    Procedure EllBoth;
    procedure Minimize;
    Procedure Substitute;     // For Substitute
    Function GetSelMas: TSelMas;
    Procedure SetSelMas(SelMas: TSelMas);
    procedure ExtractRule;    // rule extraction
    procedure RemoveUselessNonterms;
  private
    function FindSelectionCover(const node: TRE_Tree; const sm: TSelMas): TRE_Tree;
  end;

implementation

uses Main, ErrorUnit, AddDialog, PrintForm, DrawObject, ChangeSize,
        TransTransformation;

const EmptyMask: TSelMas = (0);
        //        TransGrammar;

{$R *.DFM}

function TChildForm.FindSelectionCover(const node: TRE_Tree; const sm: TSelMas): TRE_Tree;

function find(id: integer):integer;
var
        i:integer;
begin
        for i:=0 to Length(sm)-1 do
                if sm[i]=id then
                begin
                        result:=i;
                        Exit;
                end;
        result:=-1;
end;

var
        lc, rc: TRE_Tree;
        idx: integer;
        tmp: string;
begin
        tmp := node.toString(EmptyMask, false);

        idx := find(node.DrawObjId);
        if idx <> -1 then // current node is selected -> no need to find in children
        begin
                result := node;
                Exit;
        end;

        lc := nil;
        rc := nil;
        if node.left <> nil then
                lc := FindSelectionCover(node.left, sm);
        if node.right <> nil then
                rc := FindSelectionCover(node.right, sm);

        // idx = -1
        if (lc = nil) and (rc = nil) then
        begin
                result := nil; 
        end else if (lc = nil) then
        begin // (rc <> nil)
                result := rc;        
        end else if (rc = nil) then
        begin // (lc <> nil)
                result := lc;                
        end else // (rc <> nil) and (lc <> nil)
        begin
                result := node;
                // here we can provide another solution,
                // where several operations such as: 'a','b','c'
                // can be represented as X,'c' where X='a','b'.
        end;
end;

procedure TChildForm.ExtractRule;
var
        sm: TSelMas;
        SelCover: TRE_Tree;
        NewRuleStr, OldRuleStr, NewNontermName: string;
        nsp, res, resadd: integer;
begin
        sm := GetSelMas;
        // recursive tree bypass
        // result - farest form root node, with all selected nodes as children
        SelCover := FindSelectionCover(m_ActiveObject.Root, sm);
        if SelCover = nil then
                Exit;
        if SelCover = m_ActiveObject.Root then
        begin
                ShowMessage('Minimal regular expression matches whole rule');
                Exit;
        end;
        OldRuleStr := m_ActiveObject.Root.toString(EmptyMask, false);
        NewRuleStr := SelCover.toString(EmptyMask, false);
        nsp := Pos(NewRuleStr, OldRuleStr); // new string position
        if nsp = 0 then
        begin
                ShowMessage('Error: substring was not found');
                Exit;
        end;

        // nonterm new name
        AddDialogForm.Caption:='Add NonTerminal...';
        AddDialogForm.Edit1.Text:=Grammar.getFreeNonTerminalName;
        res:=AddDialogForm.ShowModal;
        resadd:=-1;
        if res=mrOK then begin
          NewNontermName:=AddDialogForm.Edit1.Text;
          resadd:=Grammar.addNonTerminalName(NewNontermName);
          if resadd = -1 then
          begin
            ERROR(Format(CantAddNonTerminal,[NewNontermName]));
            Exit;
          end
        end;
        if resadd <> -1 then
        begin
                EditArea.Text := copy(OldRuleStr, 1, nsp-1) + NewNontermName
                        + copy(OldRuleStr, nsp+Length(NewRuleStr), Length(OldRuleStr))+'.';
                //BuildButton.Click;           // Automatic display
                m_ActiveObject.setValue(EditArea.Text);

                SetActiveNonTerminal(resadd);
                //Grammar.G
                EditArea.Text:=NewRuleStr+'.';
//                BuildButton.Click;           // Automatic display
                m_ActiveObject.setValue(EditArea.Text);
                BuildButton.Click;           // Automatic display

                SetActiveNonTerminal(resadd);
        end;
end;

Procedure TChildForm.RemoveUselessNonterms;
begin
        // FindUselessNonterms
        //Grammar.FindNonTerminalName(name);
       // Grammar.
        // RemoveUselessNonterms

end;

Procedure TChildForm.Save;
begin
  Grammar.Save(Caption);
end;

Procedure TChildForm.SavePicture(const FileName:string);
begin
  OffScreen.Height:=m_ActiveObject.height;
  OffScreen.Width:=m_ActiveObject.Width;
  OffScreen.SaveToFile(FileName);
  SetActive;
end;

Procedure TChildForm.Substitute;     // For Substitute
var i: integer;
    a: TSelMas;
begin
SetLength(a,0);
for i:=0 to m_ActiveObject.DrawObj.Count-1 do
    if m_ActiveObject.DrawObj.DOItems[i].Selected then
       begin
       SetLength(a,Length(a)+1);
       a[Length(a)-1]:=i;
       end;
EditArea.Text:=m_ActiveObject.Root.toString(a, false)+'.';
BuildButton.Click;           // Automatic display
end;

Function TChildForm.GetSelMas: TSelMas;
var a: TSelMas;
    i: integer;
begin
SetLength(a,0);
for i:=0 to m_ActiveObject.DrawObj.Count-1 do
    if m_ActiveObject.DrawObj.DOItems[i].Selected then
       begin
       SetLength(a,Length(a)+1);
       a[Length(a)-1]:=i;
       end;
result:=a;
end;

Procedure TChildForm.SetSelMas(SelMas: TSelMas);
var i: integer;
begin
for i:=0 to Length(SelMas)-1 do
    m_ActiveObject.DrawObj.DOItems[SelMas[i]].Selected:=true;
end;

Procedure TChildForm.EllLeft;
var
  ss, tokname: string;
  i: integer;
  name0, body0: string;
begin
{  tokname := ActiveListBox.Items[ActiveListBox.ItemIndex] + ':';
  ss := EllimLeft(tokname + EditArea.Text);
  EditArea.Text := copy(ss, Length(tokname)+1, Length(ss));}

if ActiveListBox.Items.Count>0 then
  begin
  tokname:=ActiveListBox.Items[ActiveListBox.ItemIndex]+':';
  ss:='';
  for i:=0 to ActiveListBox.Items.Count-1 do
      begin
      name0:=ActiveListBox.Items[i]+':';
      if name0=tokname then body0:=EditArea.Text
                       else begin
                            body0:=TNTListItem(ActiveListBox.Items.Objects[i]).Value;
{  m_ActiveObject:=TNTListItem(ActiveListBox.Items.Objects[ActiveListBox.ItemIndex]);
  m_ActiveObject.setStringFromRoot;
  EditArea.Text:=m_ActiveObject.Value;}
                            end;
//      MessageDlg(name0+body0,mtInformation,[mbOk],0);
      ss:=ss+name0+body0;
      end;
//  MessageDlg(ss,mtInformation,[mbOk],0);
  ss:=EllimLeft(ss,copy(tokname,1,length(tokname)-1));
  EditArea.Text:=copy(ss,Length(tokname)+1,Length(ss));
  BuildButton.Click;           // Automatic display
  end;
end;

Procedure TChildForm.EllRight;
var
  ss, tokname: string;
  i: integer;
  name0, body0: string;
begin
{  tokname := ActiveListBox.Items[ActiveListBox.ItemIndex] + ':';
  ss := EllimLeft(tokname + EditArea.Text);
  EditArea.Text := copy(ss, Length(tokname)+1, Length(ss));}

if ActiveListBox.Items.Count>0 then
  begin
  tokname:=ActiveListBox.Items[ActiveListBox.ItemIndex]+':';
  ss:='';
  for i:=0 to ActiveListBox.Items.Count-1 do
      begin
      name0:=ActiveListBox.Items[i]+':';
      if name0=tokname then body0:=EditArea.Text
                       else begin
                            body0:=TNTListItem(ActiveListBox.Items.Objects[i]).Value;
{  m_ActiveObject:=TNTListItem(ActiveListBox.Items.Objects[ActiveListBox.ItemIndex]);
  m_ActiveObject.setStringFromRoot;
  EditArea.Text:=m_ActiveObject.Value;}
                            end;
//      MessageDlg(name0+body0,mtInformation,[mbOk],0);
      ss:=ss+name0+body0;
      end;
//  MessageDlg(ss,mtInformation,[mbOk],0);
  ss:=EllimRight(ss,copy(tokname,1,length(tokname)-1));
  EditArea.Text:=copy(ss,Length(tokname)+1,Length(ss));
  BuildButton.Click;           // Automatic display
  end;
end;


{
Procedure TChildForm.EllRight;
var
  ss, tokname: string;
  i: integer;
  name0, body0: string;
  SelMask: TSelMas;    // For Right Elimination
begin
  if ActiveListBox.Items.Count>0 then
  begin
    SetLength(SelMask, ActiveListBox.Items.Count);                         // For Right Elimination
    FillChar(SelMask, sizeof(selMask[0]) * ActiveListBox.Items.Count , 0); // For Right Elimination
    tokname:=ActiveListBox.Items[ActiveListBox.ItemIndex]+':';
    ss:='';

    for i:=0 to ActiveListBox.Items.Count-1 do
    begin
      name0:=ActiveListBox.Items[i]+':';
      //if name0=tokname then body0:=EditArea.Text
                         begin
                          // For Right Elimination
                          body0:=TNTListItem(ActiveListBox.Items.Objects[i]).Value;
                          end;
      ss:=ss+name0+body0;
    end;
    // For Right Elimination
    //ss:=EllimLeft(ss,copy(tokname,1,length(tokname)-1));
    ss:=EllimRight(ss,copy(tokname,1,length(tokname)-1));
    EditArea.Text:=copy(ss,Length(tokname)+1,Length(ss));
    BuildButton.Click;           // Automatic display

    // Transform value
    //TNTListItem(ActiveListBox.Items.Objects[ActiveListBox.ItemIndex]).setValue(copy(ss,Length(tokname)+1,Length(ss)));
    //ss:=TNTListItem(ActiveListBox.Items.Objects[ActiveListBox.ItemIndex]).Root.toString(SelMask, true);
    // Display value
    //EditArea.Text:=ss+'.';
    //BuildButton.Click;           // Automatic display
  end;
end;
}

Procedure TChildForm.EllBoth;
var
  ss, tokname: string;
  i: integer;
  name0, body0: string;
begin
if ActiveListBox.Items.Count>0 then
  begin
  tokname:=ActiveListBox.Items[ActiveListBox.ItemIndex]+':';
  ss:='';
  for i:=0 to ActiveListBox.Items.Count-1 do
      begin
      name0:=ActiveListBox.Items[i]+':';
      if name0=tokname then body0:=EditArea.Text
                       else begin
                            body0:=TNTListItem(ActiveListBox.Items.Objects[i]).Value;
                            end;
      ss:=ss+name0+body0;
      end;
  ss:=EllimBoth(ss,copy(tokname,1,length(tokname)-1));
  EditArea.Text:=copy(ss,Length(tokname)+1,Length(ss));
  BuildButton.Click;           // Automatic display
  end;
end;

procedure TChildForm.Minimize;
var
  ss, tokname: string;
  i: integer;
  name0, body0: string;
begin
if ActiveListBox.Items.Count>0 then
  begin
  tokname:=ActiveListBox.Items[ActiveListBox.ItemIndex]+':';
  ss:='';
  for i:=0 to ActiveListBox.Items.Count-1 do
      begin
      name0:=ActiveListBox.Items[i]+':';
      if name0=tokname then body0:=EditArea.Text
                       else begin
                            body0:=TNTListItem(ActiveListBox.Items.Objects[i]).Value;
                            end;
      ss:=ss+name0+body0;
      end;
  // grammardesc: ansistring; name: string
  ss:=TransTransformation.Minimize(ss, copy(tokname,1,length(tokname)-1));
  EditArea.Text:=copy(ss,Length(tokname)+1,Length(ss));
  BuildButton.Click;           // Automatic display
  end;
end;

Procedure TChildForm.PrintOffScreen;
begin
  BitmapPrintForm.ClientHeight:=m_ActiveObject.height;
  BitmapPrintForm.ClientWidth:=m_ActiveObject.Width;
  BitmapPrintForm.SetOffScreen(offScreen);
  BitmapPrintForm.ShowModal;
  BitmapPrintForm.Print;
end;

function TChildForm.getSemanticTable:TStrings;
begin
  result:=SemanticListBox.Items;
end;

Procedure TChildForm.SetActiveNonTerminal(index:Integer);
begin
  AuxilaryNotionsListBox.ItemIndex:=-1;
  ActiveListBox:=NonTerminalListBox;
  ActiveListBox.ItemIndex:=index;
  SetActive;
  BuildButton.Click;         // For substitute. Need for correct work
end;

Procedure TChildForm.SetActiveMacro(index:Integer);
begin
  NonTerminalListBox.ItemIndex:=-1;
  ActiveListBox:=AuxilaryNotionsListBox;
  ActiveListBox.ItemIndex:=index;
  SetActive;
end;
Procedure TChildForm.reDrawOffScreen;
begin
  with offScreen do begin
    Canvas.Brush.Color:=clWhite;
    Canvas.FillRect(Rect(0,0,Width,Height));
    Canvas.Font.Style:=[fsBold];
    Canvas.TextOut(5,5,ActiveListBox.Items[ActiveListBox.ItemIndex]);
    Canvas.Font.Style:=[];
    m_ActiveObject.Draw(Canvas);
    Canvas.Pen.Style:=psDot;
    Canvas.Brush.Color:=clWhite;
    Canvas.PolyLine([m_Contur.TopLeft,
                     Point(m_Contur.Right,m_Contur.Top),
                     m_Contur.BottomRight,
                     Point(m_Contur.Left,m_Contur.Bottom),
                     m_Contur.TopLeft]);
    Canvas.Pen.Style:=psSolid;
  end;
end;                                                                             

Procedure TChildForm.SetActive;
begin
  m_ActiveObject:=TNTListItem(ActiveListBox.Items.Objects[ActiveListBox.ItemIndex]);
  m_ActiveObject.setStringFromRoot;
  EditArea.Text:=m_ActiveObject.Value;
  with offScreen do begin
    if (m_ActiveObject.height>cMinPaintBoxHeight) then
      Height:=m_ActiveObject.height
    else
      Height:=cMinPaintBoxHeight;
    if (m_ActiveObject.Width>cMinPaintBoxWidth) then
      Width:=m_ActiveObject.Width
    else
      Width:=cMinPaintBoxWidth;
    ClientArea.Height:=Height;
    ClientArea.Width:=Width;
    RedrawOffScreen;
  end;
  ClientAreaPaint(nil);
end;

procedure TChildForm.FormCreate(Sender: TObject);
begin
  UndoRedo:=TUndoRedo.Create(Self);
  ClientArea.SetBounds(0,0,0,0);
  EditArea.Width:=Width-BuildButton.Width-8;
  BuildButton.Left:=EditArea.Width;
  Grammar:=TGrammar.make(Self,TerminalListBox.Items, SemanticListBox.Items,
                           NonTerminalListBox.Items, AuxilaryNotionsListBox.Items);
  offScreen:=TBitmap.Create;
  offScreen.Height:=cMinPaintBoxHeight;
  offScreen.Width:=cMinPaintBoxWidth;
end;

procedure TChildForm.ClientAreaPaint(Sender: TObject);
begin
  ClientArea.Canvas.Draw(0,0,offScreen);
end;

procedure TChildForm.FormClose(Sender: TObject;
             var Action: TCloseAction);
begin
  UndoRedo.ClearData;
  Action:=caFree;
  MainForm.DisableMenuItems;
  AnalyzeForm.Close;
end;

procedure TChildForm.BuildButtonClick(Sender: TObject);
var a: TSelMas;
begin
  a:=GetSelMas;
  m_ActiveObject.setValue(EditArea.Text);
  SetActive;
  UndoRedo.AddState(NonTerminalListBox,NonTerminalListBox.ItemIndex,a);
  AnalyzeForm.ShowAll(ActiveListBox);
end;

procedure TChildForm.NonTerminalListBoxClick(Sender: TObject);
begin
  SetActiveNonTerminal(NonTerminalListBox.ItemIndex)
end;

procedure TChildForm.AuxilaryNotionsListBoxClick(Sender: TObject);
begin
  SetActiveMacro(AuxilaryNotionsListBox.ItemIndex)
end;

procedure TChildForm.ActionAddMacroExecute(Sender: TObject);
var
  res:integer;
  NewName:string;
begin
  AddDialogForm.Caption:='Add AuxilaryNotions...';
  AddDialogForm.Edit1.Text:=Grammar.getFreeMacroName;
  res:=AddDialogForm.ShowModal;
  if res=mrOK then begin
    NewName:=AddDialogForm.Edit1.Text;
    res:=Grammar.addMacroName(NewName);
    if res = -1 then
      ERROR(Format(CantAddMacro,[NewName]))
    else
      SetActiveMacro(res);
  end;
end;

procedure TChildForm.ActionAddNonTerminalExecute(Sender: TObject);
var
  NewName:string;
  res:integer;
begin
  AddDialogForm.Caption:='Add NonTerminal...';
  AddDialogForm.Edit1.Text:=Grammar.getFreeNonTerminalName;
  res:=AddDialogForm.ShowModal;
  if res=mrOK then begin
    NewName:=AddDialogForm.Edit1.Text;
    res:=Grammar.addNonTerminalName(NewName);
    if res = -1 then
      ERROR(Format(CantAddNonTerminal,[NewName]))
    else
      SetActiveNonTerminal(res);
  end;
end;

procedure TChildForm.ActionAddSemanticExecute(Sender: TObject);
var
  NewName:string;
  res:integer;
begin
  AddDialogForm.Caption:='Add Context Symbol...';
  AddDialogForm.Edit1.Text:=Grammar.getFreeSemanticName;
  if AddDialogForm.ShowModal=mrOK then begin
    NewName:=AddDialogForm.Edit1.Text;
    res:=Grammar.addSemanticName(NewName);
    if res = -1 then
      ERROR(Format(CantAddSemantic,[NewName]))
  end;
end;

procedure TChildForm.ActionEditActiveNameExecute(Sender: TObject);
var
  res:integer;
  ListBoxType:string;
begin
  With ActiveListBox do begin
    ListBoxType:=Name;
    Delete(ListBoxType, length(ListBoxType)-6, length(ListBoxType));
    AddDialogForm.Caption:='Edit '+ListBoxType+' name';
    AddDialogForm.Edit1.Text:=Items[ItemIndex];
    res:=AddDialogForm.ShowModal;
    if res=mrOK then begin
      Items[ItemIndex]:=AddDialogForm.Edit1.Text;
    end;
  end;
end;

procedure TChildForm.ActionChangeBitmapSizeExecute(Sender: TObject);
begin
  ChangeSizeForm.HeightEdit.Text:=IntToStr(m_ActiveObject.Height);
  ChangeSizeForm.WidthEdit.Text:=IntToStr(m_ActiveObject.Width);
  if ChangeSizeForm.ShowModal=mrOK then begin
    m_ActiveObject.Height:=StrToInt(ChangeSizeForm.HeightEdit.Text);
    m_ActiveObject.Width :=StrToInt(ChangeSizeForm.WidthEdit.Text);
    setActive;
  end;
end;


procedure TChildForm.ActionOpenAllMacroExecute(Sender: TObject);
begin
  Grammar.OpenAllMacro;
  SetActive;
end;

procedure TChildForm.ActionCloseAllDefinitionExecute(Sender: TObject);
begin
  Grammar.CloseAllDefinition;
  SetActive;
end;

procedure TChildForm.ClientAreaMouseUp(Sender: TObject;
  Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  if (Button=mbLeft) then begin
    if moveDrawObjects=cMove then
      moveDrawObjects:=cNone
    else if moveDrawObjects=cContur then begin
      m_ActiveObject.ChangeSelectionInRect(m_Contur);
      m_Contur:=Rect(0,0,0,0);
      moveDrawObjects:=cNone;
    end;
    ReDrawOffScreen;
    ClientAreaPaint(nil);
  end;
end;

procedure TChildForm.ClientAreaMouseDown(Sender: TObject;
  Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
var
  Target:TDrawObject;
begin
  if (Button=mbLeft) then begin
    Target:=m_ActiveObject.FindDO(x,y);
    if Target<>nil then begin
      if (ssCtrl in Shift) then
        m_ActiveObject.ChangeSelection(Target)
      else if (ssShift in Shift) then
        m_ActiveObject.SelectAllNotSelected(Target)
      else begin
        if Target.Selected then begin
          moveDrawObjects:=cMove;
          m_ActiveObject.SelectedSetTo(X,Y);
        end else begin
          m_ActiveObject.UnSelectAll;
          m_ActiveObject.ChangeSelection(Target)
        end;
      end;
    end else begin
      if not (ssCtrl in Shift) then
        m_ActiveObject.UnSelectAll;
      moveDrawObjects:=cContur;
      m_Contur:=Rect(x,y,x,y);
    end;
    ReDrawOffScreen;
    ClientAreaPaint(nil);
  end;
end;

procedure TChildForm.ClientAreaMouseMove(Sender: TObject;
  Shift: TShiftState; X, Y: Integer);
begin
  if ssLeft in Shift then begin
    if moveDrawObjects=cMove then
      m_ActiveObject.SelectedMoveTo(X,Y)
    else if moveDrawObjects=cContur then
      m_Contur.BottomRight:=Point(x,y);
    ReDrawOffScreen;
    ClientAreaPaint(nil);
  end;
end;

procedure TChildForm.ActionAddExtendedPointExecute(Sender: TObject);
begin
  m_ActiveObject.AddExtendedPoint;
  SetActive;
end;

end.
