unit TransRE_Nonterminal;

interface
uses
  TransRE_Tree, TransRE_Leaf, classes, TransNonterminal, Minimization;
type
  TRE_Nonterminal = class(TRE_Leaf)
  private
    m_Nonterminal:TNonterminal;
  public
    { Public declarations }
    constructor Create(aGrammar:TComponent; aNonterminal:TNonterminal);reintroduce;overload;
    function equals(aTree :TRE_Tree):boolean;override;

    {String representation of the Tree}
    function getString():string;override;
    {String representation of the Tree with substituded AuxilaryNotions}
    function getSubstitutedString():string;override;

    {returns one of the Type constants}
    function getREType():integer;override;

    {contain subtree (now used only for Leafs)}
    function contain(aTree:TRE_Tree):boolean;override;

    {left recursion elimination}
    function leftEl(aLeaf:TRE_Tree):TTransformation;override;
    {right recursion elimination}
    function rightEl(aLeaf:TRE_Tree):TRightTransformation;override;
  end;

implementation
uses
  TransCreator;

constructor TRE_Nonterminal.Create(aGrammar:TComponent; aNonterminal:TNonterminal);
begin
  inherited Create(aGrammar);
  m_Nonterminal:=aNonterminal;
end;

function TRE_Nonterminal.equals(aTree :TRE_Tree):boolean;
begin
  if(aTree is TRE_Nonterminal) then
  begin
    result:=m_Nonterminal.equals((TRE_Nonterminal(aTree)).m_Nonterminal);
  end else begin
    result:=false;
  end;
end;

function TRE_Nonterminal.getREType():integer;
begin
  result:=ctNonterminal;
end;

function TRE_Nonterminal.getString():string;
begin
  result:=m_Nonterminal.getName();
end;

function TRE_Nonterminal.getSubstitutedString():string;
begin
  if (m_Nonterminal.AuxilaryNotion) then begin
    result:=m_Nonterminal.Tree.getSubstitutedString();
  end else begin
    result:=getString();
  end;                                                          
end;

function TRE_Nonterminal.contain(aTree :TRE_Tree):boolean;
begin
  if (equals(aTree)) then begin
    result:=true;
  end else if (m_Nonterminal.AuxilaryNotion) then begin
    result:=m_Nonterminal.Tree.contain(aTree);
  end else begin
    result:=false;
  end;
end;

function TRE_Nonterminal.rightEl(aLeaf:TRE_Tree):TRightTransformation;
begin
  if (equals(aLeaf)) then begin
    result.E:=false;
    result.RA:=createEmptyTerminal(Owner);
    result.RB:=nil;
  end else if (m_Nonterminal.AuxilaryNotion) then begin
    result := m_Nonterminal.Tree.rightEl(aLeaf);
    if (nil=result.RA) then begin
      result.RB := Self;
    end;
  end else begin
    result.E:=false;
    result.RA:=nil;
    result.RB:=Self;
  end;
end;

function TRE_Nonterminal.leftEl(aLeaf:TRE_Tree):TTransformation;
begin
  if (equals(aLeaf)) then begin
    result.E:=false;
    result.R1:=createEmptyTerminal(Owner);
    result.R2:=nil;
  end else if (m_Nonterminal.AuxilaryNotion) then begin
    result := m_Nonterminal.Tree.leftEl(aLeaf);
    if (nil=result.R1) then begin
      result.R2 := Self;
    end;
  end else begin
    result.E:=false;
    result.R1:=nil;
    result.R2:=Self;
  end;
end;

end.
