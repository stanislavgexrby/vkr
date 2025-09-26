unit TransRE_And;

interface
uses
  TransRE_Tree,TransRE_BinaryOperation, Minimization;

type
  TRE_And = class(TRE_BinaryOperation)
  private
    { Private declarations }
  public
    {returns one of the Type constants}
    function getREType():integer;override;

    {left recursion elimination}
    function leftEl(aLeaf:TRE_Tree):TTransformation;override;
    {right recursion elimination}
    function rightEl(aLeaf:TRE_Tree):TRightTransformation;override;
    {for }
    //function TRE_BinaryOperation.getString():string;override;
    procedure buildMinimizationTable(var table:TMinimizationTable; minRec:TMinRecord);override;
  end;

implementation
uses
  TransCreator;
function TRE_And.getREType():integer;
begin
  result:=ctAnd;
end;

function TRE_And.leftEl(aLeaf:TRE_Tree):TTransformation;
var
  LeftTr, RightTr:TTransformation;
begin
  LeftTr:=m_LeftOperand.leftEl(aLeaf);
  if (LeftTr.E) then begin
    RightTr:=m_RightOperand.leftEl(aLeaf);
    result.E:=RightTr.E;
    result.R1:=createOr(Owner, createAnd(Owner, LeftTr.R1, m_RightOperand), RightTr.R1);
    result.R2:=createOr(Owner, createAnd(Owner, LeftTr.R2, m_RightOperand), RightTr.R2);
  end else begin
    result.E:=false;
    result.R1:=createAnd(Owner, LeftTr.R1, m_RightOperand);
    result.R2:=createAnd(Owner, LeftTr.R2, m_RightOperand);
  end;
end;

function TRE_And.rightEl(aLeaf:TRE_Tree):TRightTransformation;
var
  LeftTr, RightTr:TRightTransformation;
begin
  RightTr:=m_RightOperand.rightEl(aLeaf);
  if (RightTr.E) then begin
    LeftTr:=m_LeftOperand.rightEl(aLeaf);
    result.E:=LeftTr.E;
    result.RA:=createOr(Owner, createAnd(Owner, m_LeftOperand, RightTr.RA), LeftTr.RA);
    result.RB:=createOr(Owner, createAnd(Owner, m_LeftOperand, RightTr.RB), LeftTr.RB);
  end else begin
    result.E:=false;
    result.RA:=createAnd(Owner, m_LeftOperand, RightTr.RA);
    result.RB:=createAnd(Owner, m_LeftOperand, RightTr.RB);
  end;
end;

procedure TRE_And.buildMinimizationTable(var table:TMinimizationTable; minRec:TMinRecord);
var
  finish: TState;
begin
  finish := minRec.finish; 
  minRec.finish := table.createState();
  m_LeftOperand.buildMinimizationTable(table, minRec);

  minRec.start := minRec.finish;
  minRec.finish := finish;
  m_RightOperand.buildMinimizationTable(table, minRec);
end;

end.

