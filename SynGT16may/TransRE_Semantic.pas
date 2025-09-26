unit TransRE_Semantic;

interface
uses
  TransRE_Tree, TransRE_Leaf, classes, TransSemantic;
type
  TRE_Semantic = class(TRE_Leaf)
  private
    m_Semantic:TSemantic;
  public
    { Public declarations }
    constructor Create(aGrammar:TComponent; aSemantic:TSemantic);reintroduce;overload;
    function equals(aTree :TRE_Tree):boolean;override;
    function getString:string;override;
    {left recursion elimination}
    function leftEl(aLeaf:TRE_Tree):TTransformation;override;

    {returns one of the Type constants}
    function getREType:integer;override;
  end;

implementation
constructor TRE_Semantic.Create(aGrammar:TComponent; aSemantic:TSemantic);
begin
  inherited Create(aGrammar);
  m_Semantic:=aSemantic;
end;

function TRE_Semantic.equals(aTree :TRE_Tree):boolean;
begin
  if(aTree is TRE_Semantic) then
  begin
    result:=m_Semantic.equals((TRE_Semantic(aTree)).m_Semantic);
  end
  else
  begin
    result:=false;
  end;
end;

function TRE_Semantic.getREType():integer;
begin
  result:=ctSemantic;
end;

function TRE_Semantic.getString():string;
begin
  result:=m_Semantic.getName;
end;

function TRE_Semantic.leftEl(aLeaf:TRE_Tree):TTransformation;
begin
  result.E:=false;
  result.R1:=nil;
  result.R2:=self;
end;

end.
