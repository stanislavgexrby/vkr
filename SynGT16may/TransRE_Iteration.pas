unit TransRE_Iteration;

interface
uses
  TransRE_Tree,TransRE_BinaryOperation, Minimization;

type
  TRE_Iteration = class(TRE_BinaryOperation)
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
  TransCreator, TransRE_And, TransGrammar;
function TRE_Iteration.getREType():integer;
begin
  result:=ctIteration;
end;

function TRE_Iteration.leftEl(aLeaf:TRE_Tree):TTransformation;
var
  RightTr:TTransformation;
begin
  if (m_LeftOperand.isEmpty()) then begin
    RightTr:=m_RightOperand.leftEl(aLeaf);
    result.E:=true;
    result.R1:=createAnd(Owner, RightTr.R1, self);
    result.R2:=createAnd(Owner, RightTr.R2, self);
  end else begin
    result:=(TRE_And.create(Owner, m_LeftOperand,
               createUnaryIteration(Owner,
                 createAnd(Owner, m_RightOperand,m_LeftOperand))
               )
             ).leftEl(aLeaf);
  end;
end;

function TRE_Iteration.rightEl(aLeaf:TRE_Tree):TRightTransformation;
var
  RightTr:TRightTransformation;
begin
  if (m_LeftOperand.isEmpty()) then begin
    RightTr:=m_RightOperand.rightEl(aLeaf);
    result.E:=true;
    result.RA:=createAnd(Owner, self, RightTr.RA);
    result.RB:=createAnd(Owner, self, RightTr.RB);
  end else begin
    result:=(TRE_And.create(Owner, m_LeftOperand,
               createUnaryIteration(Owner,
                 createAnd(Owner, m_RightOperand,m_LeftOperand))
               )
             ).rightEl(aLeaf);
  end;
end;
{
begin
  if (contain(aLeaf)) then begin
    raise ECantTransformateException.create('Iteration in deleting');
  end else begin
    result.RA := nil;
    result.RB := Self;
  end
end;
}

procedure TRE_Iteration.buildMinimizationTable(var table:TMinimizationTable; minRec:TMinRecord);
var
  finish: TState;
begin
  finish := minRec.finish;
  minRec.finish := table.createState();
  table.linkStates(minRec.finish, finish, Minimization.EmptySymbol);  
  m_LeftOperand.buildMinimizationTable(table, minRec);

  finish := minRec.finish;
  minRec.finish := minRec.start;
  minRec.start := finish;
  m_RightOperand.buildMinimizationTable(table, minRec);
end;

end.

