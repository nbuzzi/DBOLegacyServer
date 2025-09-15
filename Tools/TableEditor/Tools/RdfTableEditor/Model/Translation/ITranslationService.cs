using System.Threading;
using System.Threading.Tasks;

namespace RdfTableEditor.Model.Translation
{
    public interface ITranslationService
    {
        Task<string> TranslateAsync(string text, string? fromLanguage = null, string toLanguage = "en", CancellationToken ct = default);
    }
}
