unit TransRE_Terminal;
interface
uses
  TransRE_Tree, TransRE_Leaf, classes, TransTerminal, Minimization;
type
  TRE_Terminal = class(TRE_Leaf)
  private
    m_Terminal:TTerminal;
  public
    { Public declarations }
    constructor Create(aGrammar:TComponent; aTerminal:TTerminal);reintroduce;overload;
    function equals(aTree :TRE_Tree):boolean;override;
    function isEmpty:boolean;override;
    function getString:string;override;
    {left recursion elimination}
    function leftEl(aLeaf:TRE_Tree):TTransformation;override;
    {right recursion elimination}
    function rightEl(aLeaf:TRE_Tree):TRightTransformation;override;

    function getOperationCount():integer;override;

    {returns one of the Type constants}
    function getREType:integer;override;
  end;

implementation
constructor TRE_Terminal.Create(aGrammar:TComponent; aTerminal:TTerminal);
begin
  inherited Create(aGrammar);
  m_Terminal:=aTerminal;
end;

function TRE_Terminal.getREType():integer;
begin
  result:=ctTerminal;
end;

function TRE_Terminal.isEmpty():boolean;
begin
  result:=m_Terminal.isEmpty();
end;

function TRE_Terminal.equals(aTree :TRE_Tree):boolean;
begin
  if(aTree is TRE_Terminal) then
  begin
    result:=m_Terminal.equals((TRE_Terminal(aTree)).m_Terminal);
  end
  else
  begin
    result:=false;
  end;
end;

function TRE_Terminal.leftEl(aLeaf:TRE_Tree):TTransformation;
begin
  if (isEmpty()) then begin
    result.E:=true;
    result.R1:=nil;
    result.R2:=nil;
  end else begin
    result.E:=false;
    result.R1:=nil;
    result.R2:=Self;
  end;
end;

function TRE_Terminal.rightEl(aLeaf:TRE_Tree):TRightTransformation;
begin
  if (isEmpty()) then begin
    result.E:=true;
    result.RA:=nil;
    result.RB:=nil;
  end else begin
    result.E:=false;
    result.RA:=nil;
    result.RB:=Self;
  end;
end;

function TRE_Terminal.getString():string;
begin
  result:=m_Terminal.getName();
  if (Pos('''', result)=0) then
  begin
    result :=''''+result+'''';
  end
  else
  begin
    result :='"'+result+'"';
  end;
end;

function TRE_Terminal.getOperationCount():integer;
begin
  if (isEmpty()) then
    result:=0
  else
    result:=1;
end;


end.
