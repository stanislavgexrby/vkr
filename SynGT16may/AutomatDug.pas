unit AutomatDug;

interface
  T = class(TComponent)
  private
    m_Nonterminals:TNonterminalList;
    m_Terminals:TTerminalList;
{todo:   m_Semantics:TSemantisList;}
  public
    constructor load(aOwner:TComponent; filename:string);
    constructor loadFromStr(aOwner:TComponent; srcstr:string);
    {For debug}
    procedure println();
    {For debug}
    function printSubstituted(name: string):string;
    {For debug left recursion elimination}
    procedure leftEl();
    {For debug right recursion elimination}
    procedure rightEl();
    {Regularize grammar}
    procedure regularize();

    procedure minimize;

    function getNonterminal(aName:string):TNonterminal;
    function getTerminal(aName:string):TTerminal;
    function addNonterminal(aName:string):TNonterminal;
    function addTerminal(aName:string):TTerminal;
  end;

implementation

end.
 