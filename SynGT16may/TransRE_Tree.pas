unit TransRE_Tree;

{Type constants}
interface
uses
  classes, sysutils, Minimization;

const
  ctLeaf = 0;
  ctTerminal = 1;
  ctNonterminal = 2;
  ctSemantic = 3;
  ctIteration = 4;
  ctAnd = 5;
  ctOr = 6;

type
  ECantTransformateException = class(Exception);

  TRE_Tree = class;

    {record which used in left recursion elimination.
    ---A-R1---
     |      |
     --R2----
     E- means that empty can be produced.
    }
  TTransformation = record
    R1, R2 : TRE_Tree;
    E:boolean;
  end;
    {record which used in right recursion elimination.
    ---RA-A---
     |      |
     --RB----
     E- means that empty can be produced.
    }
  TRightTransformation = record
    RA, RB : TRE_Tree;
    E:boolean;
  end;

  TMinRecord = record
    start, finish: TState;
  end;

  TRE_Tree = class(TComponent)
  private
    { Private declarations }
  public
    { Public declarations }
    function equals(aTree :TRE_Tree):boolean;virtual;
    function isEmpty():boolean;virtual;

    function getOperationCount():integer;virtual;abstract;
    {String representation of the Tree}
    function getString():string;virtual;abstract;
    {String representation of the Tree with substituded AuxilaryNotions}
    function getSubstitutedString():string;virtual;abstract;
    {returns priority of the operation }
{todo:    function getPriority():integer;}
    {substitute one tree to another (now used only for Nonteminals as aTree)}
    procedure substituteAll(aTree: TRE_Tree; newTree: TRE_Tree);virtual;abstract;

    {returns one of the Type constants}
    function getREType():integer;virtual;abstract;
    {contain subtree (now used only for Leafs)}
    function contain(aTree:TRE_Tree):boolean;virtual;abstract;

    {left recursion elimination}
    function leftEl(aLeaf:TRE_Tree):TTransformation;virtual;abstract;
    {right recursion elimination}
    function rightEl(aLeaf:TRE_Tree):TRightTransformation;virtual;abstract;
    {}
    procedure buildMinimizationTable(var table:TMinimizationTable; minRec:TMinRecord);virtual;abstract;
  end;

implementation

function TRE_Tree.equals(aTree:TRE_Tree):boolean;
begin
  result:=false;
end;

function TRE_Tree.isEmpty():boolean;
begin
  result:=false;
end;

end.
