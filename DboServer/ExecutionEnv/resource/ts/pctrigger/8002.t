CNtlTSTrigger
{
	sm = 1;
	sq = 1;
	qc = 24;
	rq = 0;
	tid = 8002;
	title = 800202;

	CNtlTSGroup
	{
		gid = 0;

		CDboTSContGAct
		{
			cid = 2;
			elnk = 255;
			nextlnk = 254;
			prelnk = "1;";

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
		CDboTSContEnd
		{
			cid = 254;
			prelnk = "2;";
			type = 0;
		}
		CDboTSContUsrSel
		{
			cancellnk = 255;
			cid = 1;
			lilnk = 255;
			ndesc0 = 800121;
			uspt = -1;
			desc = 800123;
			nid0 = 2;
			type = 0;
			ust = 0;
			idx = 1381111;
			prelnk = "0;";
		}
		CDboTSContStart
		{
			cid = 0;
			stdiag = 978807;
			nolnk = 255;
			rm = 0;
			yeslnk = 1;

			CDboTSClickNPC
			{
				npcidx = "1381111;";
			}
		}
	}
}