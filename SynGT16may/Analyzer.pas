unit Analyzer;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, Buttons, Grids, ExtCtrls;

type
  TStrArray = array of string;
  TAnalyzeItem = record
                 name: string;
                 left,right,full: TStrArray;
                 go,done: boolean;
                 end;
  TAnalyzeList = array of TAnalyzeItem;

  TAnalyzeForm = class(TForm)
    Panel1: TPanel;
    Panel2: TPanel;
    Panel3: TPanel;
    NonTerminalGrid: TStringGrid;
    CloseBtn: TBitBtn;
    LeftRecBtn: TBitBtn;
    RightRecBtn: TBitBtn;
    DeleteBtn: TBitBtn;
    procedure CloseBtnClick(Sender: TObject);
    procedure LeftRecBtnClick(Sender: TObject);
    procedure RightRecBtnClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    nt_list: TAnalyzeList;
    procedure LoadList(ActiveListBox:TListBox);
    function GetLeftOperator(value0:string; ind:integer):string;
    function GetRightOperator(value0:string; ind:integer):string;
    function CutCells(value0:string):string;
    function SimpleString(value0:string):string;
    function CutEmpty(value0:string):string;
    function CutBr(value0:string):string;
    function TrimForLeft(value0:string):string;
    function TrimForRight(value0:string):string;
    function GetArray(value0:string):TStrArray;
    procedure BuldLists(i0:integer; value0:string);
    function AnalyzeOnePart(reg:integer; ind:integer):string;
    procedure AnalyzeSimple(i0:integer);
    procedure DoAnalyze;
    procedure ShowAll(ActiveListBox:TListBox);
    procedure ClearNtList;
  end;

const chrIteration: char = '#';
      chrOr: char = ';';
      chrAnd: char = ',';
      chrEnd: char = '.';
      chrOpen: char = '(';
      chrClose: char = ')';
      indirect_name='indirect';
      direct_name='direct';

var
  AnalyzeForm: TAnalyzeForm;

implementation

uses RegularExpression, Main, Child;

{$R *.DFM}

procedure TAnalyzeForm.CloseBtnClick(Sender: TObject);
begin
AnalyzeForm.Close;
end;

procedure TAnalyzeForm.LoadList(ActiveListBox:TListBox);
var i,j: integer;
begin
NonTerminalGrid.ColCount:=5;
NonTerminalGrid.RowCount:=1;
for i:=0 to ActiveListBox.Items.Count-1 do
    begin
    NonTerminalGrid.RowCount:=NonTerminalGrid.RowCount+1;
    NonTerminalGrid.Cells[0,i+1]:=ActiveListBox.Items[i];
    NonTerminalGrid.Cells[1,i+1]:=TNTListItem(ActiveListBox.Items.Objects[i]).Value;
    for j:=2 to NonTerminalGrid.ColCount-1 do
        NonTerminalGrid.Cells[j,i+1]:='';
    end;
if NonTerminalGrid.RowCount=1 then begin
                                   NonTerminalGrid.RowCount:=2;
                                   for i:=0 to NonTerminalGrid.ColCount-1 do
                                       NonTerminalGrid.Cells[i,1]:='';
                                   end;
NonTerminalGrid.Cells[0,0]:='Nonterminal';
NonTerminalGrid.Cells[1,0]:='Value';
NonTerminalGrid.Cells[2,0]:='Left recursion';
NonTerminalGrid.Cells[3,0]:='Recursion';
NonTerminalGrid.Cells[4,0]:='Right Recursion';
NonTerminalGrid.FixedRows:=1;
NonTerminalGrid.FixedCols:=0;
end;

function TAnalyzeForm.GetLeftOperator(value0:string; ind:integer):string;
var i: integer;
    br_count: integer;
    donext: boolean;
begin
br_count:=0;
i:=ind-1;
donext:=true;
while donext do
  begin
  if value0[i]=chrClose
     then inc(br_count)
     else if value0[i]=chrOpen
             then begin
                  dec(br_count);
                  if br_count<1 then donext:=false;
                  if br_count<0 then inc(i);
                  end
             else begin
                  if (value0[i]=chrOr)or
                     (value0[i]=chrAnd)or
                     (value0[i]=chrEnd)or
                     (value0[i]=chrIteration)
                         then begin
                              if br_count<1
                                 then begin
                                      donext:=false;
                                      inc(i);
                                      end;  // br_count<1..
                              end;  // if Operation..
                  end;  // if
  if i<2 then donext:=false;
  if donext then dec(i);
  end;  // while
result:=copy(value0,i,ind-i);
end;  // proc

function TAnalyzeForm.GetRightOperator(value0:string; ind:integer):string;
var i: integer;
    br_count: integer;
    donext: boolean;
begin
br_count:=0;
i:=ind+1;
donext:=true;
while donext do
  begin
  if i>=length(value0)
         then donext:=false
         else begin
              if value0[i]=chrOpen
                 then inc(br_count)
                 else if value0[i]=chrClose
                         then begin
                              dec(br_count);
                              if br_count<1 then donext:=false;
                              if br_count<0 then dec(i);
                              end
                         else begin
                              if (value0[i]=chrOr)or
                                 (value0[i]=chrAnd)or
                                 (value0[i]=chrEnd)or
                                 (value0[i]=chrIteration)
                                 then begin
                                      if br_count<1
                                         then begin
                                              donext:=false;
                                              dec(i);
                                              end;  // br_count<1..
                                      end;  // if Operation..
                              end;  // if
              end;  // if i>=length..
  if donext then inc(i);
  end;  // while
if value0[i]=chrEnd then dec(i);
result:=copy(value0,ind+1,i-ind);
end;

function TAnalyzeForm.CutCells(value0:string):string;
var i: integer;
    value1: string;
    s0: string;
    s1,s2: string;
begin
value1:=value0;
i:=1;
while i<length(value1) do
  begin
  s0:=value1[i];
  if s0=chrIteration then begin
                          // Replace:
                          s1:=GetLeftOperator(value1,i);
                          s2:=GetRightOperator(value1,i);
                          if s1='''''' then value1:=copy(value1,1,i-1-length(s1))+
                                                    chrOpen+''''''+chrOr+
                                                    chrOpen+s1+chrAnd+s2+chrAnd+s1+chrClose+chrClose+
                                                    copy(value1,i+1+length(s2),length(value1))
                                       else value1:=copy(value1,1,i-1-length(s1))+
                                                    chrOpen+s1+chrAnd+s2+chrAnd+s1+chrClose+
                                                    copy(value1,i+1+length(s2),length(value1));
                          i:=0;
                          end;
  inc(i);
  end;
result:=value1;
end;

function TAnalyzeForm.CutEmpty(value0:string):string;
var empty1,empty2: string;
    value1: string;
    i: integer;
begin
value1:=value0;
// Replace ('') => ''
i:=1;
while i<=length(value1)-3 do
  begin
  if copy(value1,i,4)='('''')'
     then begin
          value1:=copy(value1,1,i-1)+
                  ''''''+
                  copy(value1,i+4,length(value1));
          i:=0;
          end;
  inc(i);
  end;
// Remove  '',  and  ,''
empty1:=chrAnd+'''''';
empty2:=''''''+chrAnd;
i:=1;
while i<length(value1)-2 do
  begin
  if (copy(value1,i,3)=empty1)or
     (copy(value1,i,3)=empty2)
     then begin
          value1:=copy(value1,1,i-1)+copy(value1,i+3,length(value1));
          i:=0;
          end;
  inc(i);
  end;
result:=value1;
end;

function TAnalyzeForm.CutBr(value0:string):string;
var value1: string;
    i,j: integer;
    br_count: integer;
    needed: boolean;
begin
value1:=value0;
i:=1;
while i<length(value1) do
  begin
  if value1[i]=chrOpen then
     begin
     j:=i+1;
     br_count:=0;
     needed:=false;
     while j<length(value1) do
       begin
       if value1[j]=chrOr
          then begin
               if br_count<1 then needed:=true;
               end
          else if value1[j]=chrOpen
                  then inc(br_count)
                  else if value1[j]=chrClose
                          then begin
                               dec(br_count);
                               if br_count<0
                                  then begin
                                       if not(needed)
                                          then begin
                                               value1:=copy(value1,1,i-1)+
                                                       copy(value1,i+1,j-1-i)+
                                                       copy(value1,j+1,length(value1));
                                               i:=0;
                                               end;  // if remove..
                                       j:=length(value1)+1;
                                       end;  // if br_count<0..
                               end;  // if chrClose..
       inc(j);
       end;  // while j..
     end;  // if value1[i]=chrOpen..
  inc(i);
  end;  // while i..
result:=value1;
end;

function TAnalyzeForm.SimpleString(value0: string):string;
var value1: string;
begin
value1:=CutCells(value0);
value1:=CutEmpty(value1);
value1:=CutBr(value1);
result:=value1;
end;

function TAnalyzeForm.TrimForLeft(value0:string):string;
var value1: string;
    i,j: integer;
    s1,s2: string;
    itsgood: boolean;
begin
value1:=value0;
i:=1;
while i<=length(value1) do
  begin
  if value1[i]=chrAnd
     then begin
          s1:=GetLeftOperator(value1,i);
          itsgood:=false;
          for j:=1 to length(s1)-1 do
              if copy(s1,j,2)='''''' then itsgood:=true;
          if not(itsgood) then
             begin
             s2:=GetRightOperator(value1,i);
             value1:=copy(value1,1,i-1)+copy(value1,i+1+length(s2),length(value1));
             i:=0;
             end;  // if not good Operation
          end;  // if Operation=And
  inc(i);
  end;  // while
result:=value1;
end;

function TAnalyzeForm.TrimForRight(value0:string):string;
var value1: string;
    i,j: integer;
    s1,s2: string;
    itsgood: boolean;
begin
value1:=value0;
i:=length(value1);
while i>0 do
  begin
  if value1[i]=chrAnd
     then begin
          s1:=GetRightOperator(value1,i);
          itsgood:=false;
          for j:=1 to length(s1)-1 do
              if copy(s1,j,2)='''''' then itsgood:=true;
          if not(itsgood) then
             begin
             s2:=GetLeftOperator(value1,i);
             value1:=copy(value1,1,i-1-length(s2))+copy(value1,i+1,length(value1));
             i:=length(value1);
             end;  // if not good Operation
          end;  // if Operation=And
  dec(i);
  end;  // while
result:=value1;
end;

function TAnalyzeForm.GetArray(value0:string):TStrArray;
var a: TStrArray;
    oneword: string;
    i: integer;
begin
SetLength(a,0);
oneword:='';
for i:=1 to length(value0) do
  begin
  if (value0[i]=chrOr) or (value0[i]=chrAnd) or (value0[i]=chrEnd) or
     (value0[i]=chrOpen) or (value0[i]=chrClose)
     then begin
          if length(oneword)>0
             then begin
                  SetLength(a,length(a)+1);
                  a[length(a)-1]:=oneword;
                  end;
          oneword:='';
          end
     else oneword:=oneword+value0[i];
  end;  // for
result:=a;
end;

procedure TAnalyzeForm.BuldLists(i0:integer; value0:string);
var i: integer;
begin
i:=i0-1;
nt_list[i].name:=NonTerminalGrid.Cells[0,i0];
nt_list[i].full:=GetArray(value0);
nt_list[i].left:=GetArray(TrimForLeft(value0));
nt_list[i].right:=GetArray(TrimForRight(value0));
end;

function TAnalyzeForm.AnalyzeOnePart(reg:integer; ind:integer):string;
// reg = 1 - LeftRecursion, = 2 - Recursion, = 3 - RightRecursion
var i,j,q: integer;
    a: TStrArray;
    found: string;
begin
found:='';
for i:=0 to length(nt_list)-1 do
  begin
  nt_list[i].go:=false;
  nt_list[i].done:=false;
  end;
nt_list[ind].go:=true;

i:=0;
while i<length(nt_list) do
  begin
  if (nt_list[i].go)and(not(nt_list[i].done))
     then begin
          nt_list[i].done:=true;
          case reg of
          1: a:=nt_list[i].left;
          3: a:=nt_list[i].right;
          else
          a:=nt_list[i].full;
          end;  // case
          for j:=0 to length(a)-1 do
             begin
             if a[j]=nt_list[ind].name then found:=indirect_name;
             for q:=0 to length(nt_list)-1 do
                 if nt_list[q].name=a[j] then nt_list[q].go:=true;
             end;  // for
          i:=-1;
          end;  // if need processing..
  inc(i);
  end;  // while

case reg of
1: a:=nt_list[ind].left;
3: a:=nt_list[ind].right;
else
a:=nt_list[ind].full;
end;  // case
for j:=0 to length(a)-1 do
    if a[j]=nt_list[ind].name then found:=direct_name;
result:=found;
end;

procedure TAnalyzeForm.AnalyzeSimple(i0:integer);
begin
NonTerminalGrid.Cells[2,i0]:=AnalyzeOnePart(1,i0-1);  // Left
NonTerminalGrid.Cells[3,i0]:=AnalyzeOnePart(2,i0-1);  // Recur
NonTerminalGrid.Cells[4,i0]:=AnalyzeOnePart(3,i0-1);  // Right
end;

procedure TAnalyzeForm.DoAnalyze;
var i: integer;
    cur_value: string;
begin
SetLength(nt_list,NonTerminalGrid.RowCount-1);

for i:=1 to NonTerminalGrid.RowCount-1 do
    begin
    cur_value:=SimpleString(NonTerminalGrid.Cells[1,i]);
    BuldLists(i,cur_value);
    Application.ProcessMessages;
    end;  // for
for i:=1 to NonTerminalGrid.RowCount-1 do
    begin
    AnalyzeSimple(i);
    Application.ProcessMessages;
    end;  // for
ClearNtList;
end;  // proc

procedure TAnalyzeForm.ClearNtList;
var i: integer;
begin
for i:=0 to length(nt_list)-1 do
  begin
  SetLength(nt_list[i].left,0);
  SetLength(nt_list[i].right,0);
  SetLength(nt_list[i].full,0);
  end;
SetLength(nt_list,0);
end;

procedure TAnalyzeForm.ShowAll(ActiveListBox:TListBox);
begin
if AnalyzeForm.Visible then begin
                            LoadList(ActiveListBox);
                            Application.ProcessMessages;
                            DoAnalyze;
                            end;
end;

procedure TAnalyzeForm.LeftRecBtnClick(Sender: TObject);
begin
(TChildForm(MainForm.ActiveMDIChild)).SetActiveNonTerminal(NonTerminalGrid.Row-1);
try
  LeftRecBtn.Enabled:=false;
  (TChildForm(MainForm.ActiveMDIChild)).EllLeft;
finally
  LeftRecBtn.Enabled:=true;
end;
end;

procedure TAnalyzeForm.RightRecBtnClick(Sender: TObject);
begin
(TChildForm(MainForm.ActiveMDIChild)).SetActiveNonTerminal(NonTerminalGrid.Row-1);
try
  RightRecBtn.Enabled:=false;
  (TChildForm(MainForm.ActiveMDIChild)).EllRight;
finally
  RightRecBtn.Enabled:=true;
end;
end;

end.
