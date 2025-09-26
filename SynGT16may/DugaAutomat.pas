unit DugaAutomat;

interface
uses classes, Minimization, TransRE_Tree, TransGrammar;
type
  PState = ^integer;
  PDuga = ^TDuga;
  TDuga = record
    m_fromState:PState;
    m_toState:PState;
    m_tree:TRE_Tree;
  end;

  TDugaAutomat = class (TComponent)
  private
    m_dugaList:TList;
    m_stateList:TList;
    procedure mergeAllDuga;
    procedure removeState(stateIndex:integer);
    function findBestState: integer;
    function checkBinaryIteration(duga:PDuga): boolean;
  public
    constructor make(aGrammar:TGrammar; aTable:TMinimizationTable);
    procedure removeAll();
    function getRegularExpression: TRE_Tree;
    function getStateValue(state: PState): integer;
  end;

implementation
uses
  TransRE_Nonterminal, TransCreator, TransRE_Terminal;


function TDugaAutomat.findBestState: integer;
var
  bestValue, i, curValue:integer;
  state:PState;
label
  start_function;
begin
  start_function:
  result := m_stateList.Count-1;
  bestValue := 2000000000;
  for i:=m_stateList.Count-1 downto 2 do
  begin
    state:=m_stateList[i];
    curValue := getStateValue(state);
    if (curValue<bestValue) then
    begin
      if (curValue<0) then goto start_function; {Binary iteration exception}
      if (curValue=0) then begin
        result:=i;
        break; {Best found: 1 in, 1 out}
      end;
      bestValue :=curValue;
      result:=i;
    end;
  end
end;

function TDugaAutomat.getRegularExpression: TRE_Tree;
begin
  removeAll();
  result:=(PDuga(m_dugaList.First)).m_tree;
end;

function TDugaAutomat.checkBinaryIteration(duga:PDuga): boolean;
var
  i:integer;
  curDuga:PDuga;
  backDuga:PDuga;
  bFoundWrongOut, bFoundBackwardDuga:boolean;
  stateFrom, stateTo: PState;
begin
  stateFrom := duga.m_fromState;
  stateTo := duga.m_toState;
  bFoundWrongOut:=false;
  bFoundBackwardDuga:=false;
  for i:=0 to m_dugaList.Count-1 do
  begin
    curDuga := m_dugaList.Items[i];
    if (curDuga.m_fromState=stateFrom) then
    begin
      if not (curDuga.m_toState=stateTo) then
      begin
        bFoundWrongOut:=true;
        break; {exists other out}
      end;
    end;
    if (curDuga.m_toState=stateFrom) and (curDuga.m_fromState=stateTo) then
    begin
      bFoundBackwardDuga:=true;
      backDuga:=curDuga;
    end;
  end;
  if (bFoundBackwardDuga and not bFoundWrongOut) then
  begin
    duga.m_tree:=createIteration(Owner, duga.m_tree, backDuga.m_tree);
    m_dugaList.Remove(backDuga);
    Dispose(backDuga);
    result:=true;
  end
  else
    result:=false;
end;

function TDugaAutomat.getStateValue(state: PState): integer;
var
  i, inCount, outCount, inLength, outLength, roundLength:integer;
  curDuga:PDuga;
  inDuga:PDuga;
begin
    inCount := 0; outCount :=0; inLength :=0; outLength :=0; roundLength :=0;
    for i:=0 to m_dugaList.Count-1 do
    begin
      curDuga := m_dugaList.Items[i];
      if (curDuga.m_fromState=state) then
      begin
        if (curDuga.m_toState=state) then
        begin
          inc(roundLength, curDuga.m_tree.getOperationCount());
        end else
        begin
          inc(outCount);
          inc(outLength, curDuga.m_tree.getOperationCount());
        end;
      end else if (curDuga.m_toState=state) then
      begin
        inDuga := curDuga; {to check binary iteration}
        inc(inCount);
        inc(inLength, curDuga.m_tree.getOperationCount());
      end
    end;

    if (inCount<=1) And (outCount<=1) then
    begin
      result:=0;
    end else
    begin
      result:=inLength*(outCount-1)+outLength*(inCount-1)+inCount*outCount*roundLength;
      assert(result>=0);

      {Check binary iteration with exception}
      if (inCount=1) and (roundLength=0) then
      begin
        if (checkBinaryIteration(inDuga)) then
          result:=-1; {Exception, data changed: need recalculate all}
      end
    end;
end;

constructor TDugaAutomat.make(aGrammar:TGrammar; aTable:TMinimizationTable);
var
statesCount,symbolsCount, fromState,iSymbol,toState:integer;
curElement:TStatesSet;
curTree:TRE_Tree;
curDuga:^TDuga;
state:PState;
i:integer;
strName:string;
begin
    InHerited Create(aGrammar);

    statesCount := ATable.getStatesCount();
    symbolsCount := ATable.getSymbolsCount();
    m_stateList:=TList.Create();

    for i:=0 to statesCount+2 do
    begin
      New(state);
      m_stateList.Add(state)
    end;

    New(curDuga);
    curDuga^.m_fromState := m_stateList[0];
    curDuga^.m_toState := m_stateList[2];
    curDuga^.m_tree := createEmptyTerminal(aGrammar);
    m_dugaList:=TList.Create();
    m_dugaList.Add(curDuga);

    New(curDuga);
    curDuga^.m_toState := m_stateList[1];
    curDuga^.m_tree := createEmptyTerminal(aGrammar);
    for fromState:=0 to statesCount-1 do
      if (aTable.getStateName(fromState)=FinalState) then
      begin
        curDuga^.m_fromState := m_stateList[fromState+2];
        break;
      end;
    m_dugaList.Add(curDuga);

  for fromState:=0 to statesCount-1 do
  begin
    for iSymbol:=0 to symbolsCount-1 do
    begin
      curElement := ATable.getTableElement(fromState,iSymbol);
      if curElement <> nil then
      begin
        strName := aTable.getSymbol(iSymbol);
        if ((strName[1] = '"') or (strName[1] = ''''))  then
        begin
          assert(strName[Length(strName)]=strName[1]);
          curTree := TRE_Terminal.Create(aGrammar, aGrammar.getTerminal(
            Copy(strName, 2, Length(strName)-2)));
        end
        else
        begin
          curTree := TRE_Nonterminal.Create(aGrammar, aGrammar.getNonterminal(strName));
        end;

        for toState:=0 to statesCount-1 do
        if curElement.findState(toState) then
        begin
          New(curDuga);
          curDuga^.m_fromState := m_stateList[fromState+2];
          curDuga^.m_toState := m_stateList[toState+2];
          curDuga^.m_tree := curTree;
          m_dugaList.Add(curDuga);
        end;
      end;
    end;
  end;
end;

procedure TDugaAutomat.mergeAllDuga;
var
j,i:integer;
curDuga1, curDuga2:^TDuga;
begin
  i:=0;
  while (i<m_dugaList.Count) do
  begin
    curDuga1 := m_dugaList.Items[i];
    for j:=m_dugaList.Count-1 downto i+1 do
    begin
      curDuga2 := m_dugaList.Items[j];
      if (curDuga1.m_fromState=curDuga2.m_fromState)
        And (curDuga1.m_toState=curDuga2.m_toState) then
      begin
        curDuga1.m_tree := createOr(Owner, curDuga1.m_tree, curDuga2.m_tree);
        m_dugaList.Delete(j);
        Dispose(curDuga2);
      end;
    end;
    inc(i);
  end
end;

procedure TDugaAutomat.removeAll();
var
stateIndex:integer;
begin
  while (m_stateList.Count>2) do
  begin
    MergeAllDuga();
    stateIndex := FindBestState();
    RemoveState(stateIndex);
  end;
  MergeAllDuga();
end;

procedure TDugaAutomat.removeState(stateIndex:integer);
var
i,j, lastIndexBeforeRemove:integer;
curDuga1, curDuga2, newDuga:^TDuga;
iterationTree:TRE_Tree;
state:PState;
begin
  state:= m_stateList[stateIndex];
  lastIndexBeforeRemove:=m_dugaList.Count-1;
  iterationTree:=nil;
  for i:=0 to lastIndexBeforeRemove do
  begin
    curDuga1 :=m_dugaList[i];
    if (curDuga1.m_fromState=state) And (curDuga1.m_toState=state) Then
      iterationTree := curDuga1.m_tree;
  end;

  if iterationTree=nil then
    iterationTree := createEmptyTerminal(Owner)
  else
    iterationTree := createUnaryIteration(Owner, iterationTree);

  for i:=0 to lastIndexBeforeRemove do
  begin
    curDuga1 :=m_dugaList[i];
    if (curDuga1.m_fromState=state) And Not (curDuga1.m_toState=state) Then
    begin
      for j:=0 to lastIndexBeforeRemove do
      begin
        curDuga2 := m_dugaList[j];
        if Not (curDuga2.m_fromState=state) And (curDuga2.m_toState=state) Then
        begin
          New(newDuga);
          newDuga.m_fromState := curDuga2.m_fromState;
          newDuga.m_toState := curDuga1.m_toState;
          newDuga.m_tree := createAnd(Owner,
            createAnd(Owner, curDuga2.m_tree, iterationTree),
            curDuga1.m_tree);
          m_dugaList.Add(newDuga);
        end;
      end;
    end;
  end;
  for i:=0 to lastIndexBeforeRemove do
  begin
    curDuga1 :=m_dugaList[i];
    if (curDuga1.m_fromState=state) Or (curDuga1.m_toState=state) Then
    begin
      m_dugaList.Items[i]:=nil;
      Dispose(curDuga1);
    end;
  end;
  m_dugaList.Pack();

  m_stateList.Delete(stateIndex);
  Dispose(state);
end;

end.
