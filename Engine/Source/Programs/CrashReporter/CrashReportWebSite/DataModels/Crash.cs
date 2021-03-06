//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated from a template.
//
//     Manual changes to this file may cause unexpected behavior in your application.
//     Manual changes to this file will be overwritten if the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace Tools.CrashReporter.CrashReportWebSite.DataModels
{
    using System;
    using System.Collections.Generic;
    
    public partial class Crash
    {
        public int Id { get; set; }
        public string Summary { get; set; }
        public string GameName { get; set; }
        public string Status { get; set; }
        public System.DateTime TimeOfCrash { get; set; }
        public string ChangelistVersion { get; set; }
        public string PlatformName { get; set; }
        public string EngineMode { get; set; }
        public string Description { get; set; }
        public string RawCallStack { get; set; }
        public string Pattern { get; set; }
        public string CommandLine { get; set; }
        public string ComputerName { get; set; }
        public string FixedChangeList { get; set; }
        public string LanguageExt { get; set; }
        public string Module { get; set; }
        public string BuildVersion { get; set; }
        public string BaseDir { get; set; }
        public string Jira { get; set; }
        public Nullable<bool> HasLogFile { get; set; }
        public Nullable<bool> HasMiniDumpFile { get; set; }
        public Nullable<bool> HasVideoFile { get; set; }
        public string Branch { get; set; }
        public short CrashType { get; set; }
        public string SourceContext { get; set; }
        public string EpicAccountId { get; set; }
        public string EngineVersion { get; set; }
        public string UserActivityHint { get; set; }
        public int UserId { get; set; }
        public Nullable<int> PatternId { get; set; }
        public Nullable<int> ErrorMessageId { get; set; }
        public Nullable<int> BuggId { get; set; }
        public Nullable<bool> ProcessFailed { get; set; }
        public Nullable<bool> IsVanilla { get; set; }
        public string UserName { get; set; }
    
        public virtual Bugg Bugg { get; set; }
        public virtual CallStackPattern CallStackPattern { get; set; }
        public virtual ErrorMessage ErrorMessage { get; set; }
        public virtual User User { get; set; }
    }
}
