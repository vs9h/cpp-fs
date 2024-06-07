import { useState, useRef, useEffect } from "react";
import { config } from "@config/index"

const Terminal = () => {
    const [session, setSession] = useState({
        login: "guest",
        cwd: "/",
        protocol: config.fs.protocol,
        host: config.fs.host,
        port: config.fs.port,
        verbose: false,
    });

    const [history, setHistory] = useState([]);
    const [input, setInput] = useState('');
    const terminalRef = useRef(null);
    const inputRef = useRef(null);

    const getFsUrl = () => {
        return `${session.protocol}://${session.host}:${session.port}`;
    }

    const setLogin = (login) => {
        setSession(session => ({ ...session, login }));
    }

    const setCwd = (cwd) => {
        setSession(session => ({ ...session, cwd }));
    }

    const setHost = (host) => {
        setSession(session => ({ ...session, host }));
    }

    const setPort = (port) => {
        setSession(session => ({ ...session, port }));
    }

    const setVerbose = (verbose) => {
        setSession(session => ({ ...session, verbose }));
    }

    const verboseLog = (string) => {
        if (session.verbose)
            addToHistory([`*VERBOSE*: ${string}`]);
    }

    const addToHistory = (newLines) => {
        setHistory(history => [...history, ...newLines]);
    }

    const sendQuery = async (route, method = "GET") => {
        try {
            const query = `${getFsUrl()}/${route}`;
            verboseLog(`send query '${query}'`);
            console.log(`send query '${query}'`);
            const response = await fetch(query, {
                method
            });
            if (!response.ok)
                return response.json().then(error => Promise.reject(`${JSON.stringify(error, null, 1)}`));

            return response;
        } catch (error) {
            verboseLog(JSON.stringify(error, null, 1));
            return Promise.reject("internal server error");
        }
    }

    const ping = async () => sendQuery("ping").then(response => response.text());

    const ls = async (path) => sendQuery(`ls?path=${path}`).then(response => response.json()).then(object => JSON.stringify(object.entries, null, 1));

    const cat = async (path, offset, size) =>
        sendQuery(`cat?path=${path}&offset=${offset}&size=${size}`)
            .then(response => response.text());

    const store = async (dir, file, data) =>
        sendQuery(`store?dir=${dir}&file=${file}&data=${data}`, "POST")
            .then(response => response.json()).then(object => JSON.stringify(object, null, 1));

    const mkdir = async (at, dir) =>
        sendQuery(`mkdir?at=${at}&dir=${dir}`, "POST")
            .then(response => response.json()).then(object => JSON.stringify(object, null, 1));

    const squashConsecutiveSlashes = (str) =>  str.replace(/\/+/g, '/');

    const ProcessCmd = async (cmd) => {
        const [bin, ...cmds] = cmd.split(' ');
        if (bin == "login") {
            setLogin(cmds[0]);
            return null;
        }
        if (bin == "cwd") {
            setCwd(cmds[0]);
            return null;
        }
        if (bin == "cd") {
            const path = cmds[0][0] == '/' ? cmds[0] : squashConsecutiveSlashes(`${session.cwd}/${cmds[0]}`);
            return ls(path).then((_) => { setCwd(path); return null; })
        }
        if (bin == "ping") {
            return ping();
        }
        if (bin == "verbose") {
            const verbose = !session.verbose;
            setVerbose(verbose);
            return `set verbose to '${verbose}'`;
        }
        if (bin == "host") {
            setHost(cmds[0]);
            return null;
        }
        if (bin == "port") {
            setPort(cmds[0]);
            return null;
        }
        if (bin == "info") {
            return `Session info: ${JSON.stringify(session, null, 1)}`;
        }
        if (bin == "ls") {
            let path = session.cwd;
            if (cmds.length !== 0) {
                if (cmds[0][0] == '/')
                    path = cmds[0];
                else
                    path = `${path}/${cmds[0]}`
            }

            return ls(path);
        }
        if (bin === "cat") {
            if (cmds.length == 0)
                return "Please, specify path to cat";

            let path = session.cwd;
            if (cmds[0][0] == '/')
                path = cmds[0];
            else
                path = `${path}/${cmds[0]}`

            const offset = cmds.length > 1 ? cmds[1] : "";
            const size = cmds.length > 2 ? cmds[2] : "";


            return cat(path, offset, size);
        }
        if (bin === "store") {
            if (cmds.length < 2)
                return "Please, specify file name and data to store";

            const [file, ...data] = cmds;
            return store(session.cwd, file, data.join(' '));
        }
        if (bin === "mkdir") {
            if (cmds.length !== 1)
                return "Please, specify new directory name";

            return mkdir(session.cwd, cmds[0]);
        }

        return 'Command not found';
    }

    useEffect(() => {
        if (terminalRef.current) {
            terminalRef.current.scrollTop = terminalRef.current.scrollHeight;
        }
    }, [history]);

    useEffect(() => {
        if (inputRef.current) {
            inputRef.current.focus();
        }
    }, [inputRef]);

    const handleKeyDown = (e) => {
        if (e.key === 'Enter' && input != "") {
            processCommand(input);
            setInput('');
            handleTerminalClick();
        }
    };

    const handleTerminalClick = () => {
        if (inputRef.current) {
            inputRef.current.focus();
        }
    };

    const handleInputChange = (e) => {
        setInput(e.target.value);
    };


    const getInputPrefix = () => {
        const toSpan = (text, color) => <span style={{ color: color, textWrap: 'nowrap' }}>{text}</span>;
        return <>{toSpan(session.login, '#FF79C6')}:{toSpan(session.cwd, "#50FA7B")}<span style={{ whiteSpace: 'pre' }}> $ </span></>;
    }

    const processCommand = (cmd) => {
        if (cmd == "clear") {
            setHistory([])
            return;
        }

        const addCmdOutput = (cmdOut) => {
            addToHistory(cmdOut === null ? [] : [cmdOut]);
        }

        addToHistory([<>{getInputPrefix()}{cmd}</>]);
        ProcessCmd(cmd).then(addCmdOutput, addCmdOutput);
    };

    return (
        <div style={{
            fontSize: "18px",
            fontFamily: "'Fira Mono', Consolas, Menlo, Monaco, 'Courier New', Courier, monospace",
            backgroundColor: '#252a33',
            color: '#eee',
            borderRadius: '8px',
            padding: '10px',
            height: '90vh',
            width: '90vw',
            overflowY: 'auto',
            display: 'flex',
            padding: "0px 18px 15px",
            flexDirection: 'column',
        }} ref={terminalRef}
            onClick={handleTerminalClick}>
            <div style={{ textAlign: 'center', position: 'sticky', top: 0, backgroundColor: '#252a33', color: '#a2a2a2', paddingTop: '10px', zIndex: 1 }}>bash</div>
            {history.map((line, index) => (
                <div key={index} onClick={(e) => e.stopPropagation()} style={{ padding: '1px 0px', display: 'flex', alignItems: 'begin', whiteSpace: 'pre-wrap', wordBreak: 'break-all' }}>{line}</div>
            ))
            }
            <div style={{ display: 'flex', alignItems: 'center', padding: '1px 0px', }}>
                {getInputPrefix()}
                <input
                    ref={inputRef}
                    type="text"
                    value={input}
                    spellCheck="false"
                    onChange={handleInputChange}
                    onKeyDown={handleKeyDown}
                    style={{
                        fontSize: "18px",
                        fontFamily: "'Fira Mono', Consolas, Menlo, Monaco, 'Courier New', Courier, monospace",
                        flex: 1,
                        backgroundColor: '#252a33',
                        color: '#fff',
                        border: 'none',
                        outline: 'none',
                        padding: 0
                    }}
                />
            </div>
        </div >
    );
};

export default Terminal;
