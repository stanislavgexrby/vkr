unit TransRE_BinaryOperation;
interface
uses
  TransRE_Tree, classes;

const
  coOr:char =';';
  coAnd:char = ',';
  coIteration:char = '#';

type
  TRE_BinaryOperation = class(TRE_Tree)
  protected
    { Protected declarations }
    m_LeftOperand: TRE_Tree;
    m_RightOperand: TRE_Tree;
  public
    { Public declarations }
    constructor Create(aGrammar:TComponent; aLeft, aRight:TRE_Tree);reintroduce;overload;

    {substitute one tree to another (now used only for Nonteminals as aTree)}
    procedure substituteAll(aTree: TRE_Tree; newTree: TRE_Tree);override;

    {contain subtree (now used only for Leafs)}
    function contain(aTree:TRE_Tree):boolean;override;

    function getOperationCount():integer;override;

    {String representation of the Tree}
    function getString():string;override;
    {String representation of the Tree with substituded AuxilaryNotions}
    function getSubstitutedString():string;override;

    { Returns character for current operation }
    function getOperationChar:char;

{    destructor Destroy();override;}
  end;
implementation
constructor TRE_BinaryOperation.Create(aGrammar:TComponent; aLeft, aRight:TRE_Tree);
begin
  inherited Create(aGrammar);
  m_LeftOperand:=aLeft;
  m_RightOperand:=aRight;
end;
{destructor TRE_BinaryOperation.Destroy();
begin
  m_LeftOperand.Free;
  m_RightOperand.Free;
end;}

function TRE_BinaryOperation.getOperationChar():char;
begin
  case getREType of
    ctOr : result:=coOr;
    ctAnd: result:=coAnd;
    ctIteration: result:=coIteration;
  else
    result:='?';
  end;
end;

function TRE_BinaryOperation.getString():string;
var
  leftString, rightString: String;
begin
  leftString:=m_LeftOperand.getString();
  if (m_LeftOperand.getREType()>getREType()) then
    leftString:='('+leftString+')';
  rightString:=m_RightOperand.getString();
  if (m_RightOperand.getREType()>=getREType()) then
    rightString:='('+rightString+')';

  result:=leftString+getOperationChar()+rightString;
end;

function TRE_BinaryOperation.getSubstitutedString():string;
var
  leftString, rightString: String;
begin
  leftString:=m_LeftOperand.getSubstitutedString();
  if (m_LeftOperand.getREType()>getREType()) then
    leftString:='('+leftString+')';
  rightString:=m_RightOperand.getSubstitutedString();
  if (m_RightOperand.getREType()>=getREType()) then
    rightString:='('+rightString+')';

  result:=leftString+getOperationChar()+rightString;
end;

procedure TRE_BinaryOperation.substituteAll(aTree: TRE_Tree; newTree: TRE_Tree);
begin
  if (m_LeftOperand <> nil) then begin
    if (m_LeftOperand.equals(aTree)) then begin
      m_LeftOperand:=newTree;
    end else begin
      m_LeftOperand.substituteAll(aTree, newTree);
    end;
  end;

  if (m_RightOperand <> nil) then begin
    if (m_RightOperand.equals(aTree)) then begin
      m_RightOperand:=newTree;
    end else begin
      m_RightOperand.substituteAll(aTree, newTree);
    end;
  end;
end;

function TRE_BinaryOperation.contain(aTree :TRE_Tree):boolean;
begin
  result:=m_LeftOperand.contain(aTree) or m_RightOperand.contain(aTree);
end;

function TRE_BinaryOperation.getOperationCount():integer;
var
  rightResult:integer;
begin
  result:=m_LeftOperand.getOperationCount();
  if (result=0) then inc(result);
  rightResult:=m_RightOperand.getOperationCount();
  if (rightResult=0) then inc(result);
  inc(result, rightResult);
end;

end.
