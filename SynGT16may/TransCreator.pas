unit TransCreator;

interface
uses
  classes, TransRE_Tree, TransRE_Or, TransRE_And, TransRE_Iteration, TransRE_Nonterminal, TransRE_Terminal,
  TransGrammar;

function createEmptyTerminal(aGrammar:TComponent):TRE_Terminal;
function createOr(aGrammar:TComponent; aLeft, aRight:TRE_Tree):TRE_Tree;
function createOrEmpty(aGrammar:TComponent; aTree:TRE_Tree):TRE_Tree;
function createAnd(aGrammar:TComponent; aLeft, aRight:TRE_Tree):TRE_Tree;
function createAndAlt(aGrammar:TComponent; aLeft, aRight:TRE_Tree):TRE_Tree;
function createIteration(aGrammar:TComponent; aLeft, aRight:TRE_Tree):TRE_Tree;
function createUnaryIteration(aGrammar:TComponent; aTree:TRE_Tree):TRE_Tree;
function createTruncatedIteration(aGrammar:TComponent; aTree:TRE_Tree):TRE_Tree;

implementation

function createEmptyTerminal(aGrammar:TComponent):TRE_Terminal;
begin
  result:=TRE_Terminal.Create(aGrammar, (TGrammar(aGrammar)).getTerminal(''));
end;

function createOr(aGrammar:TComponent; aLeft, aRight:TRE_Tree):TRE_Tree;
begin
  if (aLeft=nil) then begin
    result:=aRight;
  end else if (aRight = nil) then begin
    result:=aLeft;
  end else begin
    result:=TRE_Or.Create(aGrammar, aLeft, aRight);
  end;
end;
function createOrEmpty(aGrammar:TComponent; aTree:TRE_Tree):TRE_Tree;
begin
  if (aTree=nil) then begin
    result:=createEmptyTerminal(aGrammar);
  end else if (aTree.isEmpty()) then begin
    result:=aTree;
  end else begin
    result:=TRE_Or.Create(aGrammar, createEmptyTerminal(aGrammar), aTree);
  end;
end;
// base variant
function createAnd(aGrammar:TComponent; aLeft, aRight:TRE_Tree):TRE_Tree;
begin
  if ((aLeft=nil)or(aRight = nil)) then
  begin
    result:=nil;
  end else if (aLeft.isEmpty()) then begin
    result:=aRight;
  end else if (aRight.isEmpty()) then begin
    result:=aLeft;
  end else begin
    result:=TRE_And.Create(aGrammar, aLeft, aRight);
  end;
end;
// this alternative variant used in TransGrammar.leftEl()
function createAndAlt(aGrammar:TComponent; aLeft, aRight:TRE_Tree):TRE_Tree;
begin
  if ((aRight = nil)and(aLeft = nil)) then begin
    result:=nil;
  end else if (aRight = nil) then begin
    result:=aLeft;
  end else if (aLeft=nil) then begin
    result:=aRight;
  end else if (aLeft.isEmpty()) then begin
    result:=aRight;
  end else if (aRight.isEmpty()) then begin
    result:=aLeft;
  end else begin
    result:=TRE_And.Create(aGrammar, aLeft, aRight);
  end;
end;

function createIteration(aGrammar:TComponent; aLeft, aRight:TRE_Tree):TRE_Tree;
begin
  if (aLeft=nil)then begin
    result:=nil;
  end else if (aRight = nil) then begin
    result:=aLeft;
  end else if ((aLeft.isEmpty())and(aRight.isEmpty())) then begin
    result:=createEmptyTerminal(aGrammar);
  end else begin
    result:=TRE_Iteration.Create(aGrammar, aLeft, aRight);
  end;
end;

function createUnaryIteration(aGrammar:TComponent; aTree:TRE_Tree):TRE_Tree;
begin
  result:=createIteration(aGrammar, createEmptyTerminal(aGrammar), aTree);
end;

function createTruncatedIteration(aGrammar:TComponent; aTree:TRE_Tree):TRE_Tree;
begin
  result:=createIteration(aGrammar, aTree, createEmptyTerminal(aGrammar));
end;

end.
