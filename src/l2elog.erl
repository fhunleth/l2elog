-module(l2elog).

-behaviour(gen_server).

%% API
-export([start_link/0]).

%% gen_server callbacks
-export([init/1, handle_call/3, handle_cast/2, handle_info/2,
         terminate/2, code_change/3]).

-record(state,
        { port :: port()
        }).

%%%===================================================================
%%% API
%%%===================================================================

%% @doc starts a pin with the given direction.
%%
%% To simplify things we aways identify the process for a pin by the
%% pin number, so there is no need to remember the Pid of a pin
%% process spawned.
%% @todo Add init for exclusive pins
%% @end
-spec start_link() ->
                    {'ok', pid()} | 'ignore' | {'error', term()}.
start_link() ->
    gen_server:start_link(?MODULE, [], []).


%%%===================================================================
%%% gen_server callbacks
%%%===================================================================


init([]) ->
    SharedLib = code:priv_dir(l2elog) ++ "/l2elog_port",
    Port = open_port({spawn_executable, SharedLib},
		     [{packet, 2}, use_stdio, in]),
    State = #state{port=Port},
    {ok, State}.

handle_call(release, _From, State) ->
    {stop, normal, ok, State}.

handle_cast(_Msg, State) ->
    {noreply, State}.

handle_info({Port, {data, [Pri | Msg]}},
            #state{port=Port}=State) ->
    % Pri from syslog is part facility and part priority
    % the low 3 bits are priority. Discard the facility.
    Priority = Pri rem 8,
    lager:dispatch_log(syslog_to_lager_priority(Priority), [], Msg, none, 256),
    {noreply, State}.

terminate(_Reason, _State) ->
    ok.

code_change(_OldVsn, State, _Extra) ->
    {ok, State}.

%%%===================================================================
%%% Internal functions
%%%===================================================================

syslog_to_lager_priority(0) -> emergency;
syslog_to_lager_priority(1) -> alert;
syslog_to_lager_priority(2) -> critical;
syslog_to_lager_priority(3) -> error;
syslog_to_lager_priority(4) -> warning;
syslog_to_lager_priority(5) -> notice;
syslog_to_lager_priority(6) -> info;
syslog_to_lager_priority(7) -> debug.
