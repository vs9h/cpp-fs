import { useState, useRef, useEffect } from "react";

const Terminal = () => {
    const [history, setHistory] = useState([]);
    const [input, setInput] = useState('');
    const terminalRef = useRef(null);
    const inputRef = useRef(null);
    const [login, setLogin] = useState("guest");
    const [cwd, setCwd] = useState("/");

    const ProcessCmd = (cmd) => {
        const cmds = cmd.split(' ');
        const bin = cmds[0]
        if (bin == "login") {
            setLogin(cmds[1]);
            return null;
        }
        if (bin == "cwd") {
            setCwd(cmds[1]);
            return null;
        }
        if (bin == "echo") {
            return cmd.substr(bin.length + 1);
        }
        if (bin == "cd") {
            setCwd(cmds[1][0] == '/' ? cmds[1] : `${cwd}/${cmds[1]}`)
            return null;
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
        return <>{toSpan(login, '#FF79C6')}:{toSpan(cwd, "#50FA7B")}<span style={{ whiteSpace: 'pre' }}> $ </span></>;
    }


    const processCommand = (cmd) => {
        if (cmd == "clear") {
            setHistory([])
            return;
        }
        const cmdOut = ProcessCmd(cmd);
        const newLines = cmdOut === null ? [] : [cmdOut];
        console.log("sethistory", history);
        setHistory([...history, <>{getInputPrefix()}{cmd}</>, ...newLines]);
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
