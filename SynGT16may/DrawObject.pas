unit DrawObject;

interface
uses
    Arrow, DrawPoint, Graphics, Classes, Windows,Semantic;
Const
  cNS_Radius=10;
  cNS_NullTerminal=2;
  cNS_Down=2;
  cHorizontalSkipFromBorder=5;
  cVerticalSkipFromBorder=5;
  cVerticalSkipForName=20;
  cNullTerminalRadius=2;

  ctNilObject=-1;
  ctDrawObjectPoint=0;
  ctDrawObjectExtendedPoint=1;
  ctDrawObjectFirst=2;
  ctDrawObjectLast=3;
  ctDrawObjectTerminal=4;
  ctDrawObjectNonTerminal=5;
  ctDrawObjectMacro=6;

type
    TDrawObjectList = class;
    TDrawObjectExtendedPoint= class;

    TDrawObject = class(TDrawPoint)
    private
        m_Selected: boolean;
        m_InArrow : TArrow;
        function getInArrow: TArrow;
        function getLength: integer; override;
        procedure setInArrow(inArrow : TArrow);
        function getFromDO:TDrawPoint;
    public
        constructor create(AOwner:TComponent);override;
        function DOType:shortint;virtual;abstract;
        procedure SaveArrows(AList:TDrawObjectList);virtual;
        Procedure LoadArrows(ATable:TStrings;AList:TDrawObjectList);virtual;
        procedure Save;virtual;
        function NeedSpike:boolean;virtual;
        procedure draw(g: TCanvas);virtual;
        procedure setPlaceToRight;
        procedure setPlaceToDown(CY:integer);
        procedure setAll(x,y : longint; inArrow : TArrow);
        procedure copyAll( drawObject : TDrawObject);

        function InternalPoint(ax,ay:integer):boolean;virtual;
        Function addExtendedPoint:TDrawObjectExtendedPoint;
        procedure SelectAllNotSelected;virtual;

        property InArrow:TArrow read getInArrow write setInArrow;
        property length: integer read getLength;
        property FromDO:TDrawPoint read getFromDO;
        property Selected: boolean read m_Selected write m_Selected;
    end;

    TDrawObjectExtendedPoint= class(TDrawObject)
    public
        function InternalPoint(ax,ay:integer):boolean;override;
        function DOType:shortint;override;
        procedure Draw(g:TCanvas);override;
    end;
    TDrawObjectPoint = class (TDrawObjectExtendedPoint)
    private
        m_InArrow2: TArrow;
    protected
        procedure setX(x: integer);override;
    public
        constructor create(AOwner:TComponent);override;
        function DOType:shortint;override;
        function NeedSpike:boolean;override;
        procedure SaveArrows(AList:TDrawObjectList);override;
        Procedure LoadArrows(ATable:TStrings;AList:TDrawObjectList);override;
        procedure Draw(g: TCanvas);override;

        procedure SelectAllNotSelected;override;

        property SecondInArrow:TArrow read m_InArrow2 write m_InArrow2;
    end;
    TDrawObjectBorder = class(TDrawObject)
    private
        m_Points: array[0..2] of TPoint;
        m_changed: boolean;
    protected
        procedure setX(x: integer);override;
        procedure setY(y: integer);override;
        procedure setPlace(x,y: integer);override;
        procedure PlacePoints;virtual;abstract;
    public
        function getLength:integer;override;
        procedure Draw(g: TCanvas);override;
    end;

    TDrawObjectFirst = class (TDrawObjectBorder)
    private
        procedure PlacePoints;override;
    public
        function DOType:shortint;override;
        procedure Place;
    end;
    TDrawObjectLast = class (TDrawObjectBorder)
    private
        procedure PlacePoints;override;
    public
        function DOType:shortint;override;
    end;
    TDrawObjectLeaf = class(TDrawObject)
    private
        m_ID : integer;
        m_Length : integer;
        procedure SetLength(s:string);
    public
        procedure Save;override;
        function getLength:integer;override;
    end;

    TDrawObjectTerminal = class(TDrawObjectLeaf)
    private
        function GetName:string;
    public
        constructor make(AOwner:TComponent; ID:integer);
        function DOType:shortint;override;
        procedure Draw(g: TCanvas);override;
    published
        property DOName:string read GetName;
    end;

    TDrawObjectNonTerminal = class(TDrawObjectLeaf)
    private
        function GetName:string;virtual;
    public
        constructor make(AOwner:TComponent; ID:integer);
        function DOType:shortint;override;
        procedure Draw(g: TCanvas);override;
    published
        property DOName:string read GetName;
    end;
    TDrawObjectMacro = class(TDrawObjectNonTerminal)
    private
        function GetName:string;override;
    public
        constructor make(AOwner:TComponent; ID:integer);
        procedure Draw(g: TCanvas);override;
        function DOType:shortint;override;
    end;

  TDrawObjectList=class(TList)
  private
    m_Height, m_Width :integer;
    m_Owner:TComponent;
    m_SelectedList : TIntegerList;
    function getDOItem(Index: Integer):TDrawObject;
    procedure setDOItem(Index: Integer;DrawObject:TDrawObject);
  public
    constructor make(AOwner:TComponent);

    Procedure Save;
    Procedure Load;

    Procedure SelectedMove(dx,dy:integer);
    Procedure UnSelectAll;
    Procedure addExtendedPoint;
    Procedure ChangeSelectionInRect(ARect:TRect);
    function FindDO(x,y:integer):TDrawObject;
    Procedure ChangeSelection(ATarget:TDrawObject);
    Procedure SelectAllNotSelected(ATarget:TDrawObject);

    procedure ClearExceptFirst;
    procedure Draw(Canvas:TCanvas);

    property Height:integer read m_Height write m_Height;
    property Width:integer read m_Width write m_Width;
    property DOItems[Index: Integer]: TDrawObject read getDOItem write setDOItem;
  end;

implementation
uses Ward, Main, Child, RegularExpression;

procedure TDrawObjectFirst.Place;
begin
  SetPlace(cHorizontalSkipFromBorder,
             cVerticalSkipFromBorder+cVerticalSkipForName+cNS_Radius);
end;
function LoadArrow(ATable:TStrings;AList:TDrawObjectList):TArrow;
var
  ArrowType:integer;
  Ward:integer;
  FromDO_ID:integer;
  Semantic:TSemanticIDList;
begin
  readln(ArrowType);
  if ArrowType<>ctNilObject then begin
    readln(Ward);
    if ArrowType=ctSemanticArrow then begin
      Semantic:=TSemanticIDList.Create(ATable);
      Semantic.Load;
    end;
    readln(FromDO_ID);
    if ArrowType=ctSemanticArrow then
      result:=TSemanticArrow.Make(Ward,AList[FromDO_ID],Semantic)
    else
      result:=TArrow.Make(Ward,AList[FromDO_ID])
  end else result:=nil;
end;

function LoadDrawObject(AForm:TComponent):TDrawObject;
var
  DOType:integer;
  x,y:integer;
  res:TDrawObject;
  ID:integer;
begin
  readln(DOType);
  readln(x);
  readln(y);
  case DOType of
    ctDrawObjectPoint:begin
      Res:=TDrawObjectPoint.Create(AForm);
    end;
    ctDrawObjectExtendedPoint:Begin
      Res:=TDrawObjectExtendedPoint.Create(AForm);
    end;
    ctDrawObjectFirst:Begin
      Res:=TDrawObjectFirst.Create(AForm);
    end;
    ctDrawObjectLast:Begin
      Res:=TDrawObjectLast.Create(AForm);
    end;
    ctDrawObjectTerminal:Begin
      readln(ID);
      Res:=TDrawObjectTerminal.make(AForm,ID);
    end;
    ctDrawObjectNonTerminal:Begin
      readln(ID);
      Res:=TDrawObjectNonTerminal.make(AForm,ID);
    end;
    ctDrawObjectMacro:Begin
      readln(ID);
      Res:=TDrawObjectMacro.make(AForm,ID);
    end;
    else
      Res:=TDrawObjectTerminal.make(AForm,0);
  end;
  res.x:=x;
  res.y:=y;
  result:=res;
end;

procedure TDrawObject.SaveArrows(AList:TDrawObjectList);
var
  PrevDO:TDrawPoint;
  PrevIndex:integer;
begin
  if m_InArrow<>nil then begin
    PrevDO:=m_InArrow.Save;
    PrevIndex:=AList.indexOf(PrevDO);
    if PrevIndex=-1 then
    ;
    writeln(AList.indexOf(PrevDO));
  end else writeln(ctNilObject);
end;
procedure TDrawObjectPoint.SaveArrows(AList:TDrawObjectList);
var
  PrevDO:TDrawPoint;
begin
  inherited SaveArrows(AList);
  if m_InArrow2<>nil then begin
    PrevDO:=m_InArrow2.Save;
    writeln(AList.indexOf(PrevDO));
  end else writeln(ctNilObject);
end;

procedure TDrawObject.LoadArrows(ATable:TStrings;AList:TDrawObjectList);
begin
  m_InArrow:=LoadArrow(ATable,AList);
end;
procedure TDrawObjectPoint.SelectAllNotSelected;
var
  FromDO:TDrawObject;
begin
  Inherited;
  if (SecondInArrow<>nil) then begin
    FromDO:=TDrawObject(SecondInArrow.getFromDO);
    if not(FromDO.Selected) then
      FromDO.SelectAllNotSelected;
  end;
end;
procedure TDrawObjectPoint.LoadArrows(ATable:TStrings;AList:TDrawObjectList);
begin
  m_InArrow:=LoadArrow(ATable,AList);
  m_InArrow2:=LoadArrow(ATable,AList);
end;
procedure TDrawObject.Save;
begin
  writeln(DOType);
  writeln(x);
  writeln(y);
end;
function TDrawObjectExtendedPoint.InternalPoint(ax,ay:integer):boolean;
begin
  result:=(Abs(x-ax)<=2)and(Abs(y-ay)<=2); 
end;
function TDrawObjectExtendedPoint.DOType:shortint;
begin
  result:=ctDrawObjectExtendedPoint;
end;
function TDrawObjectPoint.DOType:shortint;
begin
  result:=ctDrawObjectPoint;
end;
function TDrawObjectFirst.DOType:shortint;
begin
  result:=ctDrawObjectFirst;
end;
function TDrawObjectLast.DOType:shortint;
begin
  result:=ctDrawObjectLast;
end;

function TDrawObjectTerminal.DOType:shortint;
begin
  result:=ctDrawObjectTerminal;
end;
function TDrawObjectNonTerminal.DOType:shortint;
begin
  result:=ctDrawObjectNonTerminal;
end;

function TDrawObjectMacro.DOType:shortint;
begin
  result:=ctDrawObjectMacro;
end;
procedure TDrawObjectLeaf.Save;
begin
  inHerited save;
  writeln(m_ID);
end;
Procedure TDrawObjectList.Save;
var i:integer;
begin
  writeln(m_Height);
  writeln(m_Width);
  writeln(Count);
  for i:=0 to Count-1 do
    DOItems[i].Save;
  for i:=0 to Count-1 do
    DOItems[i].SaveArrows(Self);
end;

Procedure TDrawObjectList.Load;
var
  i:integer;
  DOCount:integer;
  Grammar:TGrammar;
begin
  clear;
  readln(m_Height);
  readln(m_Width);
  readln(DOCount);
  Grammar:=TGrammar(m_Owner);
  for i:=0 to DOCount-1 do
    add(LoadDrawObject(Grammar.Owner));
  for i:=0 to DOCount-1 do
    DOItems[i].LoadArrows(Grammar.SemanticList,Self);
end;
function TDrawObject.NeedSpike:boolean;
begin
  result:=true;
end;
function TDrawObjectPoint.NeedSpike:boolean;
begin
  result:=false;
end;
function TDrawObject.getFromDO:TDrawPoint;
begin
  result:=m_InArrow.getFromDO;
end;

procedure TDrawObject.setPlaceToRight;
begin
  x:=FromDO.endX+m_InArrow.getLength;
  y:=FromDO.y;
end;
procedure TDrawObject.setPlaceToDown(CY:integer);
begin
  x:=FromDO.x;
  y:=FromDO.y+CY;
end;
procedure TDrawObjectPoint.setX(x: integer);
begin
  Inherited setX(x);
  if m_InArrow2<>nil then m_InArrow2.getFromDO.x:=x;
end;

procedure TDrawObjectFirst.PlacePoints;
begin
  m_Points[0].x:=x;
  m_Points[0].y:=y-cNS_Radius;
  m_Points[1].x:=x;
  m_Points[1].y:=y+cNS_Radius;
  m_Points[2].x:=endX;
  m_Points[2].y:=y;
end;
procedure TDrawObjectLast.PlacePoints;
begin
  m_Points[0].x:=endX;
  m_Points[0].y:=y-cNS_Radius;
  m_Points[1].x:=endX;
  m_Points[1].y:=y+cNS_Radius;
  m_Points[2].x:=x;
  m_Points[2].y:=y;
end;
procedure TDrawObjectBorder.setX(X:integer);
begin
  inherited setX(X);
  m_Changed:=true;
end;
procedure TDrawObjectBorder.setY(Y:integer);
begin
  inherited setY(Y);
  m_Changed:=true;
end;
procedure TDrawObjectBorder.setPlace(x,y: integer);
begin
  inherited setPlace(x,y);
  m_Changed:=true;
end;
function TDrawObjectBorder.getLength:integer;
begin
  result:=cNS_Radius;
end;
procedure TDrawObjectBorder.Draw(g: TCanvas);
begin
  inherited Draw(g);
  if m_changed then PlacePoints;
  if m_Selected then
    g.Brush.Color:=clRed
  else
    g.Brush.Color:=clBlack;
  g.Polygon(m_Points);
end;
Procedure TDrawObjectList.addExtendedPoint;
var tempDO:TDrawObject;
begin
  if m_SelectedList.count=1 then begin
    TempDO:=DOItems[m_SelectedList[0]].AddExtendedPoint;
    if TempDO<>nil then add(TempDO);
  end;
end;
Procedure TDrawObjectList.UnSelectAll;
var i:integer;
begin
  for i:=0 to Count-1 do
    DOItems[i].Selected:=false;
  m_SelectedList.clear;
end;
function InInterval(a1,a2,a:integer):boolean;
var
  temp:integer;
begin
  if a1>a2 then begin
    temp:=a2;
    a2:=a1;
    a1:=temp;
  end;
  result:=(a>=a1)and(a<=a2);
end;
Procedure TDrawObjectList.ChangeSelectionInRect(ARect:TRect);
var i:integer;
begin
  for i:=0 to Count-1 do
    with DOItems[i] do begin
      if InInterval(ARect.Top,ARect.Bottom,y) and
            InInterval(ARect.Left,ARect.Right,x) then begin
        if Selected then begin
          m_SelectedList.Remove(i);
          Selected:=false;
        end else begin
          m_SelectedList.addToEnd(i);
          Selected:=True;
        end;
      end;
    end;
end;
function TDrawObjectList.FindDO(x,y:integer):TDrawObject;
var
  i:integer;
begin
  result:=nil;
  for i:=0 to Count-1 do
    if DOItems[i].InternalPoint(x,y) then
      result:=DOItems[i];
end;
Procedure TDrawObjectList.SelectedMove(DX,DY:integer);
var
  i:integer;
begin
  for i:=0 to m_SelectedList.count-1 do
    DOItems[m_SelectedList.IDs[i]].Move(DX,DY);
end;
Procedure TDrawObjectList.SelectAllNotSelected(ATarget:TDrawObject);
var
  I:integer;
begin
  m_SelectedList.clear;
  ATarget.SelectAllNotSelected;
  for i :=0 to count-1 do
    if DOItems[i].Selected then m_SelectedList.addToEnd(i);
end;
Procedure TDrawObjectList.ChangeSelection(ATarget:TDrawObject);
var
  I:integer;
begin
  i:=indexOf(ATarget);
  if ATarget.Selected then begin
    m_SelectedList.Remove(i);
    ATarget.Selected:=false;
  end else begin
    m_SelectedList.addToEnd(i);
    ATarget.Selected:=True;
  end;
end;
constructor TDrawObjectList.make(AOwner:TComponent);
var
  First,Last:TDrawObject;
begin
  inherited Create;
  m_Owner:=AOwner;
  m_SelectedList:=TIntegerList.Create;
  first:=TDrawObjectFirst.Create(AOwner);
  first.setPlace(cHorizontalSkipFromBorder,cVerticalSkipFromBorder);
  Last:=TDrawObjectLast.Create(AOwner);
  Last.InArrow:=TArrow.make(cwFORWARD,first);
  add(First);
  add(Last);
end;

procedure TDrawObjectList.Draw(Canvas:TCanvas);
var index:integer;
begin
  for index:=0 to count-1 do
  begin
    DOItems[index].Draw(Canvas);
  end;
end;

procedure TDrawObjectList.ClearExceptFirst;
var index:integer;
begin
  for index:=count-1 downto 1 do
  begin
    Delete(index);
  end;
end;

function TDrawObject.getInArrow: TArrow;
begin
     result:=m_InArrow;
end;
function TDrawObject.getLength: integer;
begin
     result:=0;
end;
procedure TDrawObject.setInArrow(inArrow : TArrow);
begin
     m_inArrow:=inArrow;
end;
procedure TDrawObject.setAll(x,y : longint; inArrow : TArrow);
begin
     self.x:=x;
     self.y:=y;
     m_inArrow:=inArrow;
end;
procedure TDrawObject.copyAll(drawObject : TDrawObject);
begin
     x:=drawObject.x;
     y:=drawObject.y;
     m_inArrow:=drawObject.InArrow.copy;
end;

procedure TDrawObject.Draw(g:TCanvas);
begin
  if m_InArrow<>nil then
    m_InArrow.Draw(g, self);
end;
procedure TDrawObject.SelectAllNotSelected;
var
  FromDO:TDrawObject;
begin
  Selected:=true;
  if (InArrow<>nil) then begin
    FromDO:=TDrawObject(InArrow.getFromDO);
    if not(FromDO.Selected) then
      FromDO.SelectAllNotSelected;
  end;
end;
Function TDrawObject.addExtendedPoint:TDrawObjectExtendedPoint;
var
  curWard:integer;
begin
  if InArrow<>nil then begin
    result:=TDrawObjectExtendedPoint.Create(Owner);
    if InArrow.Ward=cwFORWARD then begin
      InArrow.Ward:=cwNONE;
      curWard:=cwFORWARD;
    end else
      curWard:=cwNONE;
    result.InArrow:=InArrow;
    result.x:=((InArrow.getFromDO).endX+x)div 2;
    result.y:=y;
    InArrow:=TArrow.make(curWard,result);
  end else
    result:=nil;
end;
function TDrawObject.InternalPoint(ax,ay:integer):boolean;
begin
  result:=(ax>=x)and(ax<=endX)and(abs(ay-y)<=cNS_Radius);
end;
constructor TDrawObjectPoint.create(AOwner:TComponent);
begin
  inherited;
  m_InArrow2:=nil;
end;
procedure TDrawObjectPoint.Draw(g:TCanvas);
begin
  inherited;
  if m_InArrow2<>nil then m_InArrow2.Draw(g, self);
end;
procedure TDrawObjectExtendedPoint.Draw(g:TCanvas);
begin
  inherited;
  if m_Selected then begin
    g.Brush.Color:=clRed;
    g.Ellipse(x-2,y-2,x+2,y+2);
  end;
end;
function TDrawObjectLeaf.getLength: integer;
begin
     result:=m_length;
end;
procedure TDrawObjectLeaf.SetLength(s:string);
begin
  m_length:=(cNS_Radius*2)+NormalSizeCanvas.TextWidth(s);
end;
function TDrawObjectTerminal.GetName:string;
begin
    with Owner as TChildForm do
        result:=Grammar.GetTerminalName(m_ID);
end;
procedure TDrawObjectTerminal.Draw(g: TCanvas);
begin
  inherited Draw(g);
  if m_Selected then
    g.Brush.Color:=clRed
  else
    g.Brush.Color:=clSilver;
  if m_ID<>0 then begin
    g.RoundRect(X, Y-cNS_Radius, EndX, Y + cNS_Radius,
                 cNS_Radius, cNS_Radius);
    g.TextOut( X+cNS_Radius, Y-cNS_Radius+cNS_Down, DOName );
  end else
    g.RoundRect(X, Y-cNS_NullTerminal, EndX, Y + cNS_NullTerminal,
                 cNS_NullTerminal, cNS_NullTerminal);
end;
procedure TDrawObjectNonTerminal.Draw(g: TCanvas);
begin
  inherited Draw(g);
  if m_Selected then
    g.Brush.Color:=clRed
  else
    g.Brush.Color:=clSilver;
  g.Rectangle(X, Y-cNS_Radius, EndX, Y + cNS_Radius);
  g.TextOut( X+cNS_Radius, Y-cNS_Radius+cNS_Down, DOName );
end;
constructor TDrawObject.Create(AOwner:TComponent);
begin
  inherited Create(AOwner);
  m_Selected:=false
end;
constructor TDrawObjectTerminal.make(AOwner:TComponent;ID:integer);
begin
  Inherited Create(AOwner);
  m_ID:=ID;
  if ID<>0 then
    with Owner as TChildForm do
      SetLength(Grammar.getTerminalName(ID))
  else
    m_Length:=cNS_NullTerminal*2;
end;

function TDrawObjectNonTerminal.GetName:string;
begin
    with Owner as TChildForm do
        result:=Grammar.GetNonTerminalName(m_ID);
end;

function TDrawObjectMacro.GetName:string;
begin
    with Owner as TChildForm do
        result:=Grammar.GetMacroName(m_ID);
end;

constructor TDrawObjectNonTerminal.make(AOwner:TComponent;ID:integer);
begin
  Inherited Create(AOwner);
  m_ID:=ID;
  with Owner as TChildForm do
    SetLength(Grammar.getNonTerminalName(ID));
end;

procedure TDrawObjectMacro.Draw(g: TCanvas);
begin
  if m_InArrow<>nil then
    m_InArrow.Draw(g, self);

  if m_Selected then
    g.Brush.Color:=clRed
  else
    g.Brush.Color:=clSilver;
  g.pen.Style:=psDot;
  g.Rectangle(X, Y-cNS_Radius, EndX, Y + cNS_Radius);
  g.pen.Style:=psSolid;
  g.TextOut( X+cNS_Radius, Y-cNS_Radius+cNS_Down, DOName );
end;

constructor TDrawObjectMacro.make(AOwner:TComponent;ID:integer);
begin
  Inherited Create(AOwner);
  m_ID:=ID;
  with Owner as TChildForm do
    SetLength(Grammar.getMacroName(ID));
end;

function TDrawObjectList.GetDOItem(Index: Integer):TDrawObject;
begin
  result:=TDrawObject(Items[Index]);
end;
procedure TDrawObjectList.setDOItem(Index: Integer; DrawObject:TDrawObject);
begin
  Items[Index]:=DrawObject;
end;
end.
