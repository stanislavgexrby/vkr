unit TransRE_Or;

interface
uses
  TransRE_Tree,TransRE_BinaryOperation, classes, Minimization;

type
  TRE_Or = class(TRE_BinaryOperation)
  private
    { Private declarations }
  public
    {returns one of the Type constants}
    function getREType():integer;override;
    {left recursion elimination}
    function leftEl(aLeaf:TRE_Tree):TTransformation;override;
    {right recursion elimination}
    function rightEl(aLeaf:TRE_Tree):TRightTransformation;override;

    procedure buildMinimizationTable(var table:TMinimizationTable; minRec:TMinRecord);override;
  end;

implementation
uses
  TransCreator;
  
function TRE_Or.getREType():integer;
begin
  result:=ctOr;
end;
function TRE_Or.leftEl(aLeaf:TRE_Tree):TTransformation;
var
  LeftTr, RightTr:TTransformation;
begin
  LeftTr:=m_LeftOperand.leftEl(aLeaf);
  RightTr:=m_RightOperand.leftEl(aLeaf);
  result.E:=(LeftTr.E) or (RightTr.E);
  result.R1:=createOr(Owner, LeftTr.R1, RightTr.R1);
  result.R2:=createOr(Owner, LeftTr.R2, RightTr.R2);
end;

function TRE_Or.rightEl(aLeaf:TRE_Tree):TRightTransformation;
var
  LeftTr, RightTr:TRightTransformation;
begin
  LeftTr:=m_LeftOperand.rightEl(aLeaf);
  RightTr:=m_RightOperand.rightEl(aLeaf);
  result.E:=(LeftTr.E) or (RightTr.E);
  result.RA:=createOr(Owner, LeftTr.RA, RightTr.RA);
  result.RB:=createOr(Owner, LeftTr.RB, RightTr.RB);
end;

procedure TRE_Or.buildMinimizationTable(var table:TMinimizationTable; minRec:TMinRecord);
begin
  m_LeftOperand.buildMinimizationTable(table, minRec);
  m_RightOperand.buildMinimizationTable(table, minRec);
end;

end.
