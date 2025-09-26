unit TransParser2;
{
Grammatika razbora
RE_FromString=E'.'
E=T[';'T]*
T=F[','F]*
F=U['#'U]*
U=$Semantic; Term; NonTerm;'(' E ')'; '[' E ']'
Term=(''',NotQ,''');('"',NotDQ*,'"')
NotDQ = any char except "
NotQ = any char except '
}
interface
uses TransRE_Tree, TransCharProducer, TransParser, TransGrammar;

function RE_FromCharProducer2(ACharProducer:TCharProducer; Grammar:TGrammar): TRE_Tree;

implementation
uses TransRE_Or, TransRE_And, TransRE_Iteration, TransRE_Nonterminal, TransRE_Terminal, TransRE_Semantic,
     TransErrorUnit,SysUtils;

function E(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;forward;

function U(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;
var
  name:string;
  first: TRE_Tree;
  curChar:char;
begin
  SkipSpaces(ACharProducer);
  curChar:=ACharProducer.CurrentChar;
  case curChar of
    '(': begin
      ACharProducer.next;
      first:=E(ACharProducer,curGrammar);
      SkipSpaces(ACharProducer);
      if ACharProducer.CurrentChar=')' then begin
        ACharProducer.next;
        skipSpaces(ACharProducer);
        Result:=first;
      end else begin
        result := nil;
        ERROR(Format(CantFindChar,[')']));
        Abort;
      end;
    end;
    '[': begin
      ACharProducer.next;
      first:=E(ACharProducer,curGrammar);
      SkipSpaces(ACharProducer);
      if ACharProducer.CurrentChar=']' then begin
        ACharProducer.next;
        skipSpaces(ACharProducer);
        Result:=TRE_Or.Create(CurGrammar,
             TRE_Terminal.Create(CurGrammar, CurGrammar.getTerminal('')), first);
      end else begin
        result := nil;
        ERROR(Format(CantFindChar,[']']));
        Abort;
      end;
    end;
    '''','"': begin
      ACharProducer.next;//skip '''', '"'
      name:=readName(ACharProducer,curChar);
      if ACharProducer.next//skip '''', '"'
      then begin
        skipSpaces(ACharProducer);
        result:=TRE_Terminal.Create(CurGrammar, CurGrammar.addTerminal(name));
      end else begin
        result := nil;
        ERROR(Format(CantFindChar,[curChar]));
        Abort;
      end;
    end;
    '&','@': begin
      ACharProducer.next;
      skipSpaces(ACharProducer);
      result:=TRE_Terminal.Create(CurGrammar, CurGrammar.getTerminal(''));
    end;
    '$': begin
      ACharProducer.next;
      name:='$'+readIdentifier(ACharProducer);
{todo:      result:=CurGrammar.addSemantic(name);}
      result:=nil;
    end;
    else begin
      name:={UpperCase}(readIdentifier(ACharProducer));
      result:=TRE_Nonterminal.Create(CurGrammar, CurGrammar.addNonterminal(name))
    end;
  end;
end;

function F(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;
var
  first: TRE_Tree;
begin
  first:=U(ACharProducer,curGrammar);
  while ACharProducer.CurrentChar='#' do
  begin
    ACharProducer.next;
    first:=TRE_Iteration.Create(CurGrammar,first, U(ACharProducer,curGrammar));
  end;
  result:=first;
end;
function T(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;
var
  first: TRE_Tree;
begin
  first:=F(ACharProducer,curGrammar);
  while ACharProducer.CurrentChar=',' do
  begin
    ACharProducer.next;
    first:=TRE_And.Create(CurGrammar,first,F(ACharProducer,curGrammar));
  end;
  result:=first;
end;
function E(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;
var
  first: TRE_Tree;
begin
  first:=T(ACharProducer,curGrammar);
  while ACharProducer.CurrentChar=';' do
  begin
    ACharProducer.next;
    first:=TRE_Or.Create(CurGrammar,first,T(ACharProducer,curGrammar));
  end;
  result:=first;
end;


function RE_FromCharProducer2(ACharProducer:TCharProducer; Grammar:TGrammar): TRE_Tree;
var
  first:TRE_Tree;
begin
  first:=E(ACharProducer,Grammar);
  if ACharProducer.CurrentChar='.' then begin
    result:=first;
  end else begin
    result:=nil;
    ERROR(Format(CantFindChar,['.']));
    Abort;
  end;
end;
end.
