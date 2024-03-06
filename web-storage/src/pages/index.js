import Head from "next/head";
import Terminal from "@components/terminal.js"

export default function Home() {
  return (
    <>
      <Head>
        <title>Web storage</title>
        <meta name="description" content="Web storage" />
        <meta name="viewport" content="width=device-width, initial-scale=1" />
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <div style={
        {
          minHeight: "100vh",
          margin: 0,
          background: "#1a1e24",
          width: "100%",
          overflowY: 'hidden',
          display: "-webkit-box", display: "MsFlexbox", display: "flex",
          'WebkitBoxAlign': "center", 'MsFlexAlign': "center", alignItems: "center",
          'WebkitBoxPack': "center", 'MsFlexPack': "center", justifyContent: "center",
          'WebkitFontSmoothing': "antialiased", 'MozOsxFontSmoothing': "grayscale",
        }}>
        <Terminal />
      </div>
    </>
  );
}
