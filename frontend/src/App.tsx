import { Icon } from "@iconify/react";
import useSWR from "swr";
import axios from "axios";
import { useEffect, useState } from "react";

const fetcher = (url: string) => axios.get(url).then((res) => res.data);
const deviceName = "SmartGarden-82FA";
type StatusRes = {
  mqtt_client_name: string;
  valve: boolean[];
  moisture: boolean[];
  uptime: number;
};

function App() {
  const { data: status, isLoading } = useSWR<StatusRes>(
    `https://service-smartgarden-api.s2.tebs.co.in/sensor/${deviceName}/status`,
    fetcher,
    {
      refreshInterval: 20000,
    }
  );

  const [valves, setValves] = useState<boolean[]>([]);

  useEffect(() => {
    if (status) setValves(status.valve);
  }, [status]);

  return (
    <>
      <main className="flex min-h-screen flex-col items-center justify-between p-24">
        <div className="z-10 w-full max-w-5xl items-center justify-between font-mono text-sm lg:flex">
          <p className="fixed left-0 top-0 flex w-full justify-center border-b  bg-gradient-to-b pb-6 pt-8 backdrop-blur-2xl border-neutral-800 bg-zinc-800/30 from-inherit lg:static lg:w-auto  lg:rounded-xl lg:border lg:p-4 lg:bg-zinc-800/30">
            Automated Smart&nbsp;
            <code className="font-mono font-bold">Gardening System</code>
          </p>
          <div className="fixed bottom-0 left-0 flex h-48 w-full items-end justify-center bg-gradient-to-t from-black via-black lg:static lg:h-auto lg:w-auto lg:bg-none">
            <a
              className="pointer-events-none flex place-items-center gap-2 p-8 lg:pointer-events-auto lg:p-0"
              href="https://github.com/amjed-ali-k"
              target="_blank"
              rel="noopener noreferrer"
            >
              By Amjed Ali K
            </a>
          </div>
        </div>
        <div className="grid grid-cols-2 lg:grid-cols-5 border-b py-8 gap-12">
          {isLoading ? (
            <>
              <CardSkelton />
              <CardSkelton />
              <CardSkelton />
            </>
          ) : (
            status?.moisture.map((e, i) => <MoistureSensor id={i} alert={e} />)
          )}
        </div>
        <div className="grid grid-cols-2 lg:grid-cols-5 py-8 gap-12">
          {isLoading ? (
            <>
              <CardSkelton />
              <CardSkelton />
              <CardSkelton />
            </>
          ) : (
            valves.map((e, i) => (
              <ValveButton
                id={i}
                active={e}
                onClick={() => {
                  const newStatus = [...valves];
                  newStatus[i] = !newStatus[i];
                  setValves(newStatus);
                  axios.post(
                    `https://service-smartgarden-api.s2.tebs.co.in/sensor/${deviceName}/valve`,
                    {
                      valve: i,
                      status: newStatus[i],
                    }
                  );
                }}
              />
            ))
          )}
        </div>
        <div className="relative -z-30 flex place-items-center before:absolute before:h-[300px] before:w-[480px] before:-translate-x-1/2 before:rounded-full before:blur-2xl before:content-[''] after:absolute after:-z-20 after:h-[180px] after:w-[240px] after:translate-x-1/3 after:bg-gradient-conic after:blur-2xl after:content-[''] before:bg-gradient-to-br before:from-transparent before:to-blue-700 before:opacity-10 after:from-sky-900 after:via-[#0141ff] after:opacity-40 before:lg:h-[360px]"></div>
        <div className="mb-32 grid text-center lg:mb-0 lg:grid-cols-4 lg:text-left">
          <BottomButtons
            title="API Docs"
            description="Find in-depth information about API."
            link="https://service-smartgarden-api.s2.tebs.co.in/docs"
          />
          <BottomButtons
            title="Configuration"
            description="Configure the current system [TBD]"
            // link="https://service-smartgarden-api.s2.tebs.co.in/docs"
          />
          <BottomButtons
            title="Logs"
            description="System events and internal logs"
            // link="https://service-smartgarden-api.s2.tebs.co.in/docs"
          />
        </div>
      </main>
    </>
  );
}

export default App;

function CardSkelton() {
  return (
    <div className=" mx-auto border-2 border-slate-100/40 rounded-md min-w-[130px]">
      <div className="flex flex-col items-center h-full mt-4 animate-pulse">
        <div className="w-16 h-16 bg-gray-500 rounded-full "></div>
        <div className="flex flex-col ">
          <div className="h-2 my-4 bg-gray-500 rounded-md w-20 "></div>
        </div>
      </div>
    </div>
  );
}

function ValveButton({
  active = false,
  id,
  onClick,
}: {
  active?: boolean;
  id: number;
  onClick?: () => void;
}) {
  return (
    <div
      onClick={onClick}
      className={`border cursor-pointer min-w-[130px] duration-200 transition-all ease-in-out p-4 px-8 rounded-xl hover:bg-opacity-70 ${
        active ? "bg-green-700" : "bg-transparent hover:bg-indigo-950"
      }`}
    >
      <div className="relative">
        <div className="">
          <Icon
            className="w-16 h-16 relative"
            icon="material-symbols:water-do-outline"
          />
        </div>
        {active && (
          <div className="absolute flex items-center justify-center h-full inset-0 ">
            <Icon
              className="w-12 h-12 animate-ping "
              icon="material-symbols:water-do-outline"
            />
          </div>
        )}
      </div>
      <p className="text-sm text-center pb-2 pt-1">Valve {id}</p>
    </div>
  );
}

function MoistureSensor({
  id,
  alert = false,
}: {
  alert?: boolean;
  id: number;
}) {
  return (
    <div className="border-slate-100/40 min-w-[130px] border rounded-xl rounded-t-none overflow-hidden">
      <div className="bg-indigo-600 px-2 text-xs gap-4 text-white flex justify-between py-1 w-full">
        <p>Moisture sensor</p>
        <p>{id}</p>
      </div>
      <div className="p-8 pb-0">
        <Icon
          className={`w-16 h-16 ${alert ? "text-red-500" : "text-green-500"}`}
          icon={
            alert
              ? "line-md:alert-circle"
              : "line-md:circle-to-confirm-circle-transition"
          }
        />
      </div>
      <div
        className={`${
          alert ? "visible" : "invisible"
        } text-xs text-center p-2 text-red-500`}
      >
        Dectected
      </div>
    </div>
  );
}

function BottomButtons({
  title,
  link,
  description,
  onClick,
}: {
  title: string;
  link?: string;
  description: string;
  onClick?: () => void;
}) {
  return (
    <a
      href={link}
      onClick={onClick}
      className="group rounded-lg border border-transparent px-5 py-4 transition-colors hover:bg-gray-100 hover:border-neutral-700 hover:bg-neutral-800/30"
      target="_blank"
      rel="noopener noreferrer"
    >
      <h2 className={`mb-3 text-2xl font-semibold flex items-center `}>
        {title}{" "}
        <span className="inline-block ml-2 transition-transform group-hover:translate-x-1 motion-reduce:transform-none">
          <Icon icon="line-md:arrow-small-right" />
        </span>
      </h2>
      <p className={`m-0 max-w-[30ch] text-sm opacity-50`}>{description}</p>
    </a>
  );
}
