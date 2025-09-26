unit Semantic;

interface
uses Classes, Graphics;
const
  spaceSize=3;
type
  PSemanticID=^integer;
{  TSemantic=record
      ID:integer;
      next:PSemantic;
  end;}
  TIntegerList=class
  private
    m_Items: TList;
  public
    Constructor Create;
    procedure clear;
    procedure Remove(Number:integer);
    procedure addToBegin(ID:integer);
    procedure addToEnd(ID:integer);

    function getID(index:integer):integer;
    procedure setID(index:integer; value:integer);
    function getCount:integer;

    property IDs[Index: Integer]: integer read GetID write SetID;default;
    property Count:integer read getCount;
  end;
  TSemanticIDList = class (TIntegerList)
  private
    m_Table: TStrings;
  public
    Constructor Create(Table: TStrings);overload;
    Procedure Save;
    Procedure Load;

    function Copy:TSemanticIDList;

    function getLength:integer;
    procedure Draw(Canvas:TCanvas; x,y:integer);
  end;

implementation
Uses Main;
procedure TIntegerList.Remove(Number:integer);
var i:integer;
begin
  for i :=0 to count-1 do
    if IDs[i]=Number then begin
      m_Items.Delete(i);
      break;
    end;
end;

procedure TIntegerList.clear;
begin
  m_Items.Clear;
end;

Procedure TSemanticIDList.Save;
var i:integer;
begin
  writeln(count);
  for i :=0 to count-1 do
    writeln(IDs[i]);
end;

Procedure TSemanticIDList.Load;
var i:integer;
  IDCount:integer;
  curID:integer;
begin
  m_Items.Clear;
  readln(IDCount);
  for i :=0 to IDCount-1 do begin
    readln(curID);
    addToEnd(curID);
  end;
end;

Constructor TSemanticIDList.Create(Table: TStrings);
begin
  m_Items:=TList.Create;
  m_Table:=Table;
end;
Constructor TIntegerList.Create;
begin
  m_Items:=TList.Create;
end;
procedure TIntegerList.addToBegin(ID:integer);
var
  tempSemantic:PSemanticID;
begin
  tempSemantic:=new(PSemanticID);
  tempSemantic^:=ID;
  m_Items.Insert(0,tempSemantic);
end;

procedure TIntegerList.addToEnd(ID:integer);
var
  tempSemantic:PSemanticID;
begin
  tempSemantic:=new(PSemanticID);
  tempSemantic^:=ID;
  m_Items.add(tempSemantic);
end;

function TIntegerList.getCount:integer;
begin
  result:=m_Items.Count;
end;

function TSemanticIDList.Copy:TSemanticIDList;
var
  res:TSemanticIDList;
  index:integer;
begin
  res:=TSemanticIDList.Create(m_Table);
  for index:=0 to m_Items.Count-1 do
    res.addToEnd(integer(m_Items[index]^));
  result:=res;
end;
function TIntegerList.getID(index:integer):integer;
begin
  result:=integer(m_Items[index]^);
end;
procedure TIntegerList.setID(index:integer; value:integer);
begin
  integer(m_Items[index]^):=value;
end;
function TSemanticIDList.getLength:integer;
var
  index, res:integer;
begin
  res:=0;
  for index:=0 to m_Items.Count-2 do begin
    inc(res,NormalSizeCanvas.TextWidth(m_Table[integer(m_Items[index]^)]+','));
    inc(res,spaceSize);
  end;
  inc(res,NormalSizeCanvas.TextWidth(m_Table[integer(m_Items[Count-1]^)]+','));
  result:=res;
end;
procedure TSemanticIDList.Draw(Canvas:TCanvas; x,y:integer);
var
  index:integer;
begin
  Canvas.Brush.Color:=clWhite;
  Canvas.Brush.Style:=bsClear;
  for index:=0 to m_Items.Count-2 do begin
    Canvas.TextOut(x,y,m_Table[integer(m_Items[index]^)]+',');
    x:=Canvas.PenPos.x+spaceSize;
  end;
  Canvas.TextOut(x,y,m_Table[integer(m_Items[Count-1]^)]);
  Canvas.Brush.Style:=bsSolid;
end;
end.
