CNtlTSTrigger
{
	sm = 1;
	sq = 0;
	qc = -1;
	rq = 0;
	tid = 8002;
	title = 800202;

	CNtlTSGroup
	{
		gid = 0;

		CDboTSContGAct
		{
			cid = 4;
			elnk = 255;
			nextlnk = 100;
			prelnk = "3;";

			CDboTSActPortal
			{
				dx = "-0.105";
				px = "5883.685";
				widx = 1;
				dy = "0.000000";
				py = "-60.918";
				taid = 1;
				type = 1;
				dz = "-0.995";
				pz = "-505.653";
			}
		}
		CDboTSContStart
		{
			cid = 0;
			stdiag = 800207;
			nolnk = 253;
			rm = 0;
			yeslnk = 1;

			CDboTSCheckLvl
			{
				maxlvl = 100;
				minlvl = 30;
			}
			CDboTSClickNPC
			{
				npcidx = "1381111;";
			}
			CDboTSCheckClrQst
			{
				flink = 0;
				flinknextqid = "3701;";
				not = 0;
			}
		}
		CDboTSContEnd
		{
			cid = 254;
			prelnk = "100;";
			type = 1;
		}
		CDboTSContProposal
		{
			cancellnk = 253;
			cid = 1;
			cont = 800208;
			gtype = 1;
			oklnk = 2;
			area = 800201;
			goal = 800204;
			sort = 800205;
			prelnk = "0;";
			ds = 1;
			grade = 0;
			rwd = 100;
			title = 800202;
		}
		CDboTSContGAct
		{
			cid = 2;
			elnk = 253;
			nextlnk = 3;
			prelnk = "1;";

			CDboTSActNPCConv
			{
				conv = 800209;
				ctype = 1;
				idx = 1381111;
				taid = 2;
			}
			CDboTSActRegQInfo
			{
				cont = 800208;
				gtype = 1;
				area = 800201;
				goal = 800204;
				grade = -1;
				rwd = 100;
				scitem = -1;
				sort = 800205;
				stype = 2;
				taid = 1;
				title = 800202;
			}
		}
		CDboTSContReward
		{
			canclnk = 255;
			cid = 100;
			rwdzeny = 0;
			desc = 800214;
			nextlnk = 254;
			rwdexp = 0;
			rwdtbl = 800201;
			rwdtype = 0;
			ltime = -1;
			prelnk = "4;";
			usetbl = 1;

			CDboTSClickNPC
			{
				npcidx = "1381112;";
			}
		}
		CDboTSContEnd
		{
			cid = 253;
			prelnk = "0;1;2;";
			type = 0;
		}
		CDboTSContGCond
		{
			cid = 3;
			prelnk = "2;";
			nolnk = 255;
			rm = 0;
			yeslnk = 4;

			CDboTSClickNPC
			{
				npcidx = "1381111;";
			}
		}
	}
}

